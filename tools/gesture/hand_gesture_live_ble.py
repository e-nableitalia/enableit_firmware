#!/usr/bin/env python3
"""
Hand Gesture Live Controller with optional BLE output.

Architecture preserved from the original implementation:
- MediaPipe Tasks HandLandmarker
- LIVE_STREAM mode
- asynchronous result callback
- OpenCV rendering loop

Optional BLE mode sends textual commands to the firmware:
    sethand <thumb> <index> <middle> <ring> <pinky>

Finger values are integers in [0, 100]:
    0   = fully open
    100 = fully closed
"""

from __future__ import annotations

import argparse
import asyncio
import sys
import threading
import time
import traceback
from dataclasses import dataclass
from typing import Callable, Optional

import cv2
import mediapipe as mp
import numpy as np
from mediapipe.tasks import python
from mediapipe.tasks.python import vision

try:
    from bleak import BleakClient, BleakScanner
except ImportError:  # Allows camera-only mode even when bleak is not installed.
    BleakClient = None  # type: ignore[assignment]
    BleakScanner = None  # type: ignore[assignment]


# ── Configuration ─────────────────────────────────────────────────────────────
MODEL_PATH = "hand_landmarker.task"
CAMERA_INDEX = 0
NUM_HANDS = 2
MIN_HAND_DETECTION_CONFIDENCE = 0.5
MIN_HAND_PRESENCE_CONFIDENCE = 0.5
MIN_TRACKING_CONFIDENCE = 0.5

# Angle-based flexion is the default and recommended mode.
USE_ANGLE_FLEXION = True

# BLE send timeout. Kept short so BLE hiccups do not freeze the video loop.
SEND_TIMEOUT_SECONDS = 0.2

BLE_SERVICE_UUID = "89d60870-9908-4472-8f8c-e5b3e6573cd1"
DEFAULT_COMMAND_CHAR_UUID = "39dea685-a63e-44b2-8819-9a202581f8fe"
DEFAULT_DEVICE_NAME = "KinetiX"

MARGIN = 10               # pixels from bounding box top
FONT_SIZE = 0.9
FONT_THICKNESS = 2
HANDEDNESS_COLOR = (88, 205, 54)   # vibrant green (BGR)
LANDMARK_COLOR = (0, 128, 255)     # orange-blue (BGR)
CONNECTION_COLOR = (255, 255, 255) # white (BGR)
SELECTED_COLOR = (0, 255, 255)     # yellow (BGR)

# Finger joints: (base/MCP, PIP, DIP, TIP) landmark indices
FINGER_JOINTS = {
    "Thumb":  (1,  2,  3,  4),
    "Index":  (5,  6,  7,  8),
    "Middle": (9,  10, 11, 12),
    "Ring":   (13, 14, 15, 16),
    "Pinky":  (17, 18, 19, 20),
}
FINGER_ORDER = ["Thumb", "Index", "Middle", "Ring", "Pinky"]

# Hand connections as index pairs (MediaPipe canonical topology)
HAND_CONNECTIONS = [
    (0, 1), (1, 2), (2, 3), (3, 4),          # thumb
    (0, 5), (5, 6), (6, 7), (7, 8),          # index
    (5, 9), (9, 10), (10, 11), (11, 12),     # middle
    (9, 13), (13, 14), (14, 15), (15, 16),   # ring
    (13, 17), (17, 18), (18, 19), (19, 20),  # pinky
    (0, 17),                                  # palm
]
# ──────────────────────────────────────────────────────────────────────────────


@dataclass
class DeviceTarget:
    """BLE device target specification."""
    name: Optional[str] = None
    address: Optional[str] = None


@dataclass
class SmoothedFingers:
    """Smoothed values and send-state for the five fingers."""
    values: list[float]
    last_sent_ints: Optional[list[int]] = None


def _clamp_int(value: float, low: int = 0, high: int = 100) -> int:
    return int(max(low, min(high, value)))


def flexion_to_ints(flexion: dict[str, float]) -> list[int]:
    """Return [thumb, index, middle, ring, pinky] as clamped integers."""
    return [_clamp_int(flexion[name]) for name in FINGER_ORDER]


def build_sethand_command(finger_ints: list[int]) -> str:
    """Build 'sethand T I M R P'."""
    if len(finger_ints) != 5:
        raise ValueError("Expected exactly 5 finger values")
    return "sethand " + " ".join(str(_clamp_int(v)) for v in finger_ints)


def apply_smoothing(current: list[float], previous: list[float], alpha: float) -> list[float]:
    """Exponential smoothing: alpha*current + (1-alpha)*previous."""
    return [alpha * c + (1.0 - alpha) * p for c, p in zip(current, previous)]


def should_send(current_ints: list[int], previous_ints: Optional[list[int]], deadband: int) -> bool:
    """First command always sends; later commands send only if a finger exceeds deadband."""
    if previous_ints is None:
        return True
    return any(abs(c - p) > deadband for c, p in zip(current_ints, previous_ints))


class BleGestureController:
    """Minimal BLE client for writing textual gesture commands."""

    def __init__(self, on_status: Callable[[str], None]):
        self._on_status = on_status
        self._lock = threading.RLock()
        self._loop: Optional[asyncio.AbstractEventLoop] = None
        self._thread: Optional[threading.Thread] = None
        self._client = None
        self._char_uuid: Optional[str] = None
        self.connected = False

    def _status(self, message: str) -> None:
        try:
            self._on_status(message)
        except Exception:
            traceback.print_exc(file=sys.stdout)

    def start(self) -> None:
        if self._loop is not None:
            return

        def runner() -> None:
            loop = asyncio.new_event_loop()
            asyncio.set_event_loop(loop)
            with self._lock:
                self._loop = loop
            loop.run_forever()
            try:
                pending = asyncio.all_tasks(loop)
                for task in pending:
                    task.cancel()
                loop.run_until_complete(asyncio.gather(*pending, return_exceptions=True))
            except Exception:
                traceback.print_exc(file=sys.stdout)
            loop.close()

        self._thread = threading.Thread(target=runner, daemon=True)
        self._thread.start()
        time.sleep(0.1)

    def stop(self) -> None:
        with self._lock:
            loop = self._loop
            self._loop = None

        if loop is None:
            return

        try:
            asyncio.run_coroutine_threadsafe(self._disconnect_async(), loop).result(timeout=2.0)
        except Exception:
            traceback.print_exc(file=sys.stdout)
        loop.call_soon_threadsafe(loop.stop)

    async def _find_device(self, target: DeviceTarget, timeout: float):
        if BleakScanner is None:
            raise RuntimeError("bleak is not installed. Install it with: pip install bleak")

        if target.address:
            device = await BleakScanner.find_device_by_address(target.address, timeout=timeout)
            if not device:
                raise RuntimeError(f"Device not found at address {target.address}")
            return device

        target_name = (target.name or "").strip()
        if not target_name:
            raise RuntimeError("Device name or address required")

        devices = await BleakScanner.discover(timeout=timeout)

        # Prefer devices advertising the expected service UUID, but fall back to name-only
        # because some platforms do not expose advertisement UUIDs consistently.
        advertised_matches = []
        name_matches = []
        for device in devices:
            device_name = (device.name or "").strip()
            uuids = device.metadata.get("uuids", []) if hasattr(device, "metadata") else []
            has_service = BLE_SERVICE_UUID.lower() in [str(u).lower() for u in uuids]
            name_ok = device_name == target_name or device_name.lower() == target_name.lower()
            if name_ok and has_service:
                advertised_matches.append(device)
            elif name_ok:
                name_matches.append(device)

        if advertised_matches:
            return advertised_matches[0]
        if name_matches:
            return name_matches[0]

        raise RuntimeError(f"Device '{target_name}' not found")

    async def _ensure_services(self, client):
        if hasattr(client, "get_services"):
            try:
                await client.get_services()
            except TypeError:
                client.get_services()
        return client.services

    async def _pick_char_uuid(self, client, requested_uuid: Optional[str]) -> str:
        services = await self._ensure_services(client)

        if requested_uuid:
            wanted = str(requested_uuid).lower()
            available = []
            for service in services:
                for char in service.characteristics:
                    available.append(str(char.uuid))
                    if str(char.uuid).lower() == wanted:
                        props = set(char.properties or [])
                        if "write" in props or "write-without-response" in props:
                            return str(char.uuid)
                        raise RuntimeError(f"Characteristic {wanted} is not writeable")
            raise RuntimeError(
                f"Characteristic {wanted} not found. Available characteristics: "
                + ", ".join(sorted(set(available)))
            )

        for service in services:
            for char in service.characteristics:
                props = set(char.properties or [])
                if "write" in props or "write-without-response" in props:
                    return str(char.uuid)

        raise RuntimeError("No writeable characteristic found")

    async def _connect_async(self, target: DeviceTarget, timeout: float, char_uuid: Optional[str]) -> None:
        if BleakClient is None:
            raise RuntimeError("bleak is not installed. Install it with: pip install bleak")

        device = await self._find_device(target, timeout)
        self._status(f"Connecting to {device.address} ({device.name})...")

        client = BleakClient(device)
        await client.connect(timeout=timeout)
        selected_char_uuid = await self._pick_char_uuid(client, char_uuid or DEFAULT_COMMAND_CHAR_UUID)

        with self._lock:
            self._client = client
            self._char_uuid = selected_char_uuid
            self.connected = True

        self._status(f"Connected. Using characteristic: {selected_char_uuid}")

    async def _disconnect_async(self) -> None:
        with self._lock:
            client = self._client
            self._client = None
            self._char_uuid = None
            self.connected = False

        if client:
            try:
                self._status("Disconnecting...")
                await client.disconnect()
            except Exception:
                traceback.print_exc(file=sys.stdout)
            self._status("Disconnected.")

    async def _send_command_async(self, command: str) -> bool:
        with self._lock:
            client = self._client
            char_uuid = self._char_uuid

        if not client or not char_uuid:
            return False

        try:
            await client.write_gatt_char(char_uuid, command.encode("utf-8"), response=False)
            return True
        except Exception:
            traceback.print_exc(file=sys.stdout)
            return False

    def connect(self, target: DeviceTarget, timeout: float, char_uuid: Optional[str]):
        if self._loop is None:
            raise RuntimeError("BLE loop not started")
        return asyncio.run_coroutine_threadsafe(
            self._connect_async(target, timeout, char_uuid),
            self._loop,
        )

    def disconnect(self) -> None:
        if self._loop is None:
            return
        asyncio.run_coroutine_threadsafe(self._disconnect_async(), self._loop)

    def send_command(self, command: str) -> bool:
        if self._loop is None:
            return False
        future = asyncio.run_coroutine_threadsafe(self._send_command_async(command), self._loop)
        try:
            return bool(future.result(timeout=SEND_TIMEOUT_SECONDS))
        except Exception:
            traceback.print_exc(file=sys.stdout)
            return False


class HandGestureLive:
    """Manages webcam capture, MediaPipe inference, OpenCV rendering, and optional BLE output."""

    def __init__(
        self,
        *,
        enable_ble: bool = False,
        device_name: str = DEFAULT_DEVICE_NAME,
        device_address: str = "",
        char_uuid: str = DEFAULT_COMMAND_CHAR_UUID,
        send_rate: int = 10,
        deadband: int = 3,
        smooth_alpha: float = 0.35,
        handedness: str = "any",
        on_lost: str = "hold",
    ):
        self._result_lock = threading.Lock()
        self._latest_result: Optional[vision.HandLandmarkerResult] = None
        self._latest_timestamp_ms: Optional[int] = None
        self._last_processed_timestamp_ms: Optional[int] = None

        self.enable_ble = enable_ble
        self.device_name = device_name
        self.device_address = device_address
        self.char_uuid = char_uuid
        self.send_rate = send_rate
        self.deadband = deadband
        self.smooth_alpha = smooth_alpha
        self.handedness = handedness.lower()
        self.on_lost = on_lost
        self.min_send_interval = 1.0 / send_rate if send_rate > 0 else 0.0

        self.ble_status = "disabled" if not enable_ble else "idle"
        self.last_command_sent = ""
        self.last_send_time = 0.0
        self.smoothed: Optional[SmoothedFingers] = None
        self._last_hand_detected = False
        self._lost_sent = False

        self.ble: Optional[BleGestureController] = None
        if self.enable_ble:
            self.ble = BleGestureController(self._on_ble_status)
            self.ble.start()
            self._connect_ble()

    # ── BLE ───────────────────────────────────────────────────────────────────
    def _on_ble_status(self, message: str) -> None:
        self.ble_status = message
        print(f"[BLE] {message}")

    def _connect_ble(self) -> None:
        if not self.ble:
            return

        target = DeviceTarget(
            name=self.device_name if not self.device_address else None,
            address=self.device_address or None,
        )

        def connect_thread() -> None:
            try:
                future = self.ble.connect(target, timeout=10.0, char_uuid=self.char_uuid)
                future.result(timeout=15.0)
            except Exception as exc:
                traceback.print_exc(file=sys.stdout)
                self._on_ble_status(f"Connect failed: {exc}")

        threading.Thread(target=connect_thread, daemon=True).start()

    def _send_sethand(self, finger_ints: list[int], *, reason: str = "") -> bool:
        command = build_sethand_command(finger_ints)
        if not self.ble or not self.ble.connected:
            return False

        sent = self.ble.send_command(command)
        if sent:
            self.last_command_sent = command
            self.last_send_time = time.time()
            if self.smoothed:
                self.smoothed.last_sent_ints = list(finger_ints)
            suffix = f" ({reason})" if reason else ""
            print(f"[SEND] {command}{suffix}")
        return sent

    # ── MediaPipe callback (called from internal thread) ──────────────────────
    def _on_result(
        self,
        result: vision.HandLandmarkerResult,
        output_image: mp.Image,
        timestamp_ms: int,
    ) -> None:
        with self._result_lock:
            self._latest_result = result
            self._latest_timestamp_ms = timestamp_ms

    def _get_latest_result(self) -> tuple[Optional[vision.HandLandmarkerResult], Optional[int]]:
        with self._result_lock:
            return self._latest_result, self._latest_timestamp_ms

    # ── Finger flexion ─────────────────────────────────────────────────────────
    @staticmethod
    def _finger_flexion(hand_landmarks) -> dict[str, float]:
        """
        Returns flexion per finger as 0.0 (fully extended) – 100.0 (fist).
        Angle-based mode is default/recommended.
        Distance-based mode is kept only as a legacy/fallback option.
        """
        if USE_ANGLE_FLEXION:
            return HandGestureLive._finger_flexion_angle(hand_landmarks)
        return HandGestureLive._finger_flexion_distance(hand_landmarks)

    @staticmethod
    def _finger_flexion_angle(hand_landmarks) -> dict[str, float]:
        """
        Computed from the average of the angles at the PIP and DIP joints.
        Straight finger → ~180° angle → 0%; fist → ~90° angle → 100%.
        """
        def _angle(a, b, c) -> float:
            """Angle in degrees at joint b between vectors b→a and b→c."""
            va = np.array([a.x - b.x, a.y - b.y, a.z - b.z])
            vc = np.array([c.x - b.x, c.y - b.y, c.z - b.z])
            denom = np.linalg.norm(va) * np.linalg.norm(vc)
            if denom < 1e-6:
                return 180.0
            cos_a = float(np.clip(np.dot(va, vc) / denom, -1.0, 1.0))
            return float(np.degrees(np.arccos(cos_a)))

        result = {}
        for name, (base, pip_i, dip_i, tip_i) in FINGER_JOINTS.items():
            if name == "Thumb":
                mcp_angle = _angle(hand_landmarks[base], hand_landmarks[pip_i], hand_landmarks[dip_i])
                ip_angle = _angle(hand_landmarks[pip_i], hand_landmarks[dip_i], hand_landmarks[tip_i])
                avg_angle = (mcp_angle + ip_angle) / 2.0
            else:
                pip_angle = _angle(hand_landmarks[base], hand_landmarks[pip_i], hand_landmarks[dip_i])
                dip_angle = _angle(hand_landmarks[pip_i], hand_landmarks[dip_i], hand_landmarks[tip_i])
                avg_angle = (pip_angle + dip_angle) / 2.0

            flex = max(0.0, min(100.0, (180.0 - avg_angle) / 90.0 * 100.0))
            result[name] = round(flex, 1)
        return result

    @staticmethod
    def _finger_flexion_distance(hand_landmarks) -> dict[str, float]:
        """Legacy/fallback distance-based flexion heuristic."""
        def distance_3d(a, b) -> float:
            return float(np.sqrt((a.x - b.x) ** 2 + (a.y - b.y) ** 2 + (a.z - b.z) ** 2))

        def finger_flexion(base: int, mid: int, tip: int) -> float:
            d_base_mid = distance_3d(hand_landmarks[base], hand_landmarks[mid])
            d_mid_tip = distance_3d(hand_landmarks[mid], hand_landmarks[tip])
            d_base_tip = distance_3d(hand_landmarks[base], hand_landmarks[tip])
            if d_base_tip <= 1e-6:
                return 0.0
            ratio = (d_base_mid + d_mid_tip) / d_base_tip - 1.0
            return max(0.0, min(100.0, ratio * 50.0))

        return {
            "Thumb": round(finger_flexion(2, 3, 4), 1),
            "Index": round(finger_flexion(6, 7, 8), 1),
            "Middle": round(finger_flexion(10, 11, 12), 1),
            "Ring": round(finger_flexion(14, 15, 16), 1),
            "Pinky": round(finger_flexion(18, 19, 20), 1),
        }

    # ── Hand selection and command processing ─────────────────────────────────
    def _select_hand_index(self, result: vision.HandLandmarkerResult) -> Optional[int]:
        if not result or not result.hand_landmarks or not result.handedness:
            return None

        if self.handedness == "any":
            return 0

        for index, handedness in enumerate(result.handedness):
            label = handedness[0].category_name.lower() if handedness else ""
            if label == self.handedness:
                return index
        return None

    def _process_selected_flexion(self, flexion: dict[str, float]) -> None:
        current = [float(flexion[name]) for name in FINGER_ORDER]

        if self.smoothed is None:
            # First valid frame: initialize directly to current values, not artificial zero.
            self.smoothed = SmoothedFingers(values=list(current), last_sent_ints=None)
        else:
            self.smoothed.values = apply_smoothing(current, self.smoothed.values, self.smooth_alpha)

        current_ints = [_clamp_int(v) for v in self.smoothed.values]
        now = time.time()

        if now - self.last_send_time < self.min_send_interval:
            return

        if not should_send(current_ints, self.smoothed.last_sent_ints, self.deadband):
            return

        if self.enable_ble:
            self._send_sethand(current_ints)
        else:
            # Camera-only mode: update HUD as a preview of what would be sent.
            self.last_command_sent = build_sethand_command(current_ints)
            self.smoothed.last_sent_ints = list(current_ints)
            self.last_send_time = now

    def _handle_lost_hand(self) -> None:
        if self.on_lost != "open":
            return
        if not self._last_hand_detected or self._lost_sent:
            return

        command_ints = [0, 0, 0, 0, 0]
        sent = self._send_sethand(command_ints, reason="on-lost") if self.enable_ble else False

        # Avoid retrying continuously every frame if BLE is disconnected or the send fails.
        self._lost_sent = True

        if sent:
            self.last_command_sent = build_sethand_command(command_ints)
            if self.smoothed:
                self.smoothed.last_sent_ints = list(command_ints)
            self.last_send_time = time.time()

    # ── Drawing ───────────────────────────────────────────────────────────────
    @staticmethod
    def _draw_landmarks(
        bgr_frame: np.ndarray,
        result: vision.HandLandmarkerResult,
        selected_index: Optional[int] = None,
    ) -> np.ndarray:
        annotated = bgr_frame.copy()
        h, w = annotated.shape[:2]

        for hand_idx, (hand_landmarks, handedness) in enumerate(
            zip(result.hand_landmarks, result.handedness)
        ):
            pts = [(int(lm.x * w), int(lm.y * h)) for lm in hand_landmarks]

            line_color = SELECTED_COLOR if hand_idx == selected_index else CONNECTION_COLOR

            for start_idx, end_idx in HAND_CONNECTIONS:
                cv2.line(annotated, pts[start_idx], pts[end_idx], line_color, 2, cv2.LINE_AA)

            for pt in pts:
                cv2.circle(annotated, pt, 5, LANDMARK_COLOR, -1, cv2.LINE_AA)
                cv2.circle(annotated, pt, 5, line_color, 1, cv2.LINE_AA)

            xs = [p[0] for p in pts]
            ys = [p[1] for p in pts]
            text_x = max(0, min(xs))
            text_y = max(MARGIN, min(ys) - MARGIN)
            label = handedness[0].category_name
            if hand_idx == selected_index:
                label += " *"

            cv2.putText(
                annotated,
                label,
                (text_x, text_y),
                cv2.FONT_HERSHEY_DUPLEX,
                FONT_SIZE,
                HANDEDNESS_COLOR,
                FONT_THICKNESS,
                cv2.LINE_AA,
            )

            flexion = HandGestureLive._finger_flexion(hand_landmarks)
            panel_x = 10 + hand_idx * 220
            panel_y = h - 10 - len(flexion) * 28
            bar_w, bar_h = 120, 14

            for row, (fname, fval) in enumerate(flexion.items()):
                row_y = panel_y + row * 28
                cv2.putText(
                    annotated,
                    f"{fname[:3]}",
                    (panel_x, row_y + bar_h),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.5,
                    HANDEDNESS_COLOR,
                    1,
                    cv2.LINE_AA,
                )

                bx = panel_x + 40
                cv2.rectangle(annotated, (bx, row_y), (bx + bar_w, row_y + bar_h), (50, 50, 50), -1)

                filled = int(bar_w * fval / 100.0)
                bar_color = (int(fval * 2.55), int((100 - fval) * 2.55), 0)
                if filled > 0:
                    cv2.rectangle(annotated, (bx, row_y), (bx + filled, row_y + bar_h), bar_color, -1)

                cv2.putText(
                    annotated,
                    f"{fval:5.1f}%",
                    (bx + bar_w + 4, row_y + bar_h),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.45,
                    CONNECTION_COLOR,
                    1,
                    cv2.LINE_AA,
                )

        return annotated

    def _draw_controller_hud(self, frame: np.ndarray, hand_detected: bool, fps: float) -> np.ndarray:
        y = 30
        cv2.putText(frame, f"FPS: {fps:.1f}", (10, y), cv2.FONT_HERSHEY_SIMPLEX, 0.75, (0, 255, 0), 2, cv2.LINE_AA)
        y += 28

        ble_line = f"BLE: {'connected' if self.ble and self.ble.connected else self.ble_status}"
        cv2.putText(frame, ble_line, (10, y), cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 255, 255), 1, cv2.LINE_AA)
        y += 22

        hand_line = f"Hand: {'detected' if hand_detected else 'not detected'} ({self.handedness})"
        cv2.putText(frame, hand_line, (10, y), cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 255, 255), 1, cv2.LINE_AA)
        y += 22

        if self.smoothed:
            vals = [_clamp_int(v) for v in self.smoothed.values]
        else:
            vals = [0, 0, 0, 0, 0]
        finger_line = f"Fingers: T={vals[0]} I={vals[1]} M={vals[2]} R={vals[3]} P={vals[4]}"
        cv2.putText(frame, finger_line, (10, y), cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 255, 255), 1, cv2.LINE_AA)
        y += 22

        if self.last_command_sent:
            cv2.putText(frame, f"Last: {self.last_command_sent}", (10, y), cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 255, 255), 1, cv2.LINE_AA)

        cv2.putText(frame, "Press 'q' to exit", (10, frame.shape[0] - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.55, (0, 200, 200), 1, cv2.LINE_AA)
        return frame

    # ── Main loop ─────────────────────────────────────────────────────────────
    def run(self, camera_index: int = CAMERA_INDEX) -> None:
        base_options = python.BaseOptions(model_asset_path=MODEL_PATH)
        options = vision.HandLandmarkerOptions(
            base_options=base_options,
            running_mode=vision.RunningMode.LIVE_STREAM,
            num_hands=NUM_HANDS,
            min_hand_detection_confidence=MIN_HAND_DETECTION_CONFIDENCE,
            min_hand_presence_confidence=MIN_HAND_PRESENCE_CONFIDENCE,
            min_tracking_confidence=MIN_TRACKING_CONFIDENCE,
            result_callback=self._on_result,
        )

        cap = cv2.VideoCapture(camera_index)
        if not cap.isOpened():
            raise RuntimeError(f"Cannot open camera index {camera_index}")

        start_time = time.time()

        try:
            with vision.HandLandmarker.create_from_options(options) as detector:
                print(f"Hand gesture detection running (camera {camera_index}) — press 'q' to quit.")
                if self.enable_ble:
                    print(f"BLE enabled: name={self.device_name!r}, address={self.device_address!r}, char={self.char_uuid}")

                while True:
                    ret, bgr_frame = cap.read()
                    if not ret:
                        print("Warning: failed to read frame, retrying…")
                        continue

                    timestamp_ms = int((time.time() - start_time) * 1000)

                    rgb_frame = cv2.cvtColor(bgr_frame, cv2.COLOR_BGR2RGB)
                    mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)
                    detector.detect_async(mp_image, timestamp_ms)

                    result, result_timestamp_ms = self._get_latest_result()
                    selected_index = self._select_hand_index(result) if result else None
                    hand_detected = selected_index is not None

                    # Process each MediaPipe result only once; otherwise smoothing would be applied
                    # repeatedly to the same stale result while waiting for the next callback.
                    if result and result_timestamp_ms != self._last_processed_timestamp_ms:
                        self._last_processed_timestamp_ms = result_timestamp_ms
                        if selected_index is not None:
                            selected_landmarks = result.hand_landmarks[selected_index]
                            selected_flexion = self._finger_flexion(selected_landmarks)
                            self._process_selected_flexion(selected_flexion)
                            self._lost_sent = False
                        else:
                            self._handle_lost_hand()

                        self._last_hand_detected = hand_detected

                    display = self._draw_landmarks(bgr_frame, result, selected_index) if result else bgr_frame

                    elapsed = time.time() - start_time
                    fps = timestamp_ms / elapsed / 1000 if elapsed > 0 else 0.0
                    display = self._draw_controller_hud(display, hand_detected, fps)

                    cv2.imshow("Hand Gesture Live", display)
                    if cv2.waitKey(1) & 0xFF == ord("q"):
                        break
        finally:
            cap.release()
            cv2.destroyAllWindows()
            if self.ble:
                self.ble.disconnect()
                time.sleep(0.2)
                self.ble.stop()
            print("Stopped.")


def _list_cameras(max_check: int = 8) -> list[int]:
    """Return indices of all cameras that OpenCV can open."""
    available = []
    for index in range(max_check):
        cap = cv2.VideoCapture(index)
        if cap.isOpened():
            available.append(index)
            cap.release()
    return available


def main() -> None:
    parser = argparse.ArgumentParser(description="Hand gesture live detection with optional BLE hand control")
    parser.add_argument("--camera", "-c", type=int, default=None, help="Camera index to use (default: 0)")
    parser.add_argument("--list-cameras", "-l", action="store_true", help="List available camera indices and exit")

    parser.add_argument("--ble", action="store_true", help="Enable BLE output")
    parser.add_argument("--device-name", default=DEFAULT_DEVICE_NAME, help="BLE device name to connect to")
    parser.add_argument("--device-address", default="", help="BLE device address. If set, overrides device-name")
    parser.add_argument("--char-uuid", default=DEFAULT_COMMAND_CHAR_UUID, help="BLE characteristic UUID for sethand commands")

    parser.add_argument("--send-rate", type=int, default=10, help="Maximum BLE messages per second (0=unlimited)")
    parser.add_argument("--deadband", type=int, default=3, help="Minimum finger change needed to send update")
    parser.add_argument("--smooth-alpha", type=float, default=0.35, help="Smoothing factor in [0,1]; 1=raw")
    parser.add_argument("--handedness", default="any", choices=["any", "left", "right"], help="Which hand to control")
    parser.add_argument("--on-lost", default="hold", choices=["hold", "open"], help="Action when selected hand is lost")

    args = parser.parse_args()

    if args.list_cameras:
        cameras = _list_cameras()
        if cameras:
            print("Available cameras:", ", ".join(str(cam) for cam in cameras))
        else:
            print("No cameras found.")
        return

    if not (0.0 <= args.smooth_alpha <= 1.0):
        parser.error("--smooth-alpha must be in [0.0, 1.0]")
    if args.deadband < 0:
        parser.error("--deadband must be >= 0")
    if args.send_rate < 0:
        parser.error("--send-rate must be >= 0")
    if args.ble and (BleakClient is None or BleakScanner is None):
        parser.error("--ble requires bleak. Install it with: pip install bleak")

    camera_index = args.camera if args.camera is not None else CAMERA_INDEX
    app = HandGestureLive(
        enable_ble=args.ble,
        device_name=args.device_name,
        device_address=args.device_address,
        char_uuid=args.char_uuid,
        send_rate=args.send_rate,
        deadband=args.deadband,
        smooth_alpha=args.smooth_alpha,
        handedness=args.handedness,
        on_lost=args.on_lost,
    )
    app.run(camera_index=camera_index)


if __name__ == "__main__":
    main()
