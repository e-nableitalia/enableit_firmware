"""
Real-time hand gesture detection using MediaPipe Hand Landmarker.
Captures from webcam, detects hand landmarks and handedness, and renders results live.

Requirements:
    pip install mediapipe opencv-python numpy

Model:
    Download hand_landmarker.task from:
    https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task
"""

import argparse
import time
import threading

import cv2
import mediapipe as mp
import numpy as np
from mediapipe.tasks import python
from mediapipe.tasks.python import vision

# ── Configuration ─────────────────────────────────────────────────────────────
MODEL_PATH = "hand_landmarker.task"
CAMERA_INDEX = 0
NUM_HANDS = 2
MIN_HAND_DETECTION_CONFIDENCE = 0.5
MIN_HAND_PRESENCE_CONFIDENCE = 0.5
MIN_TRACKING_CONFIDENCE = 0.5

MARGIN = 10               # pixels from bounding box top
FONT_SIZE = 0.9
FONT_THICKNESS = 2
HANDEDNESS_COLOR = (88, 205, 54)   # vibrant green (BGR)
LANDMARK_COLOR = (0, 128, 255)     # orange-blue (BGR)
CONNECTION_COLOR = (255, 255, 255) # white (BGR)

# Finger joints: (base/MCP, PIP, DIP, TIP) landmark indices
FINGER_JOINTS = {
    "Thumb":  (1,  2,  3,  4),
    "Index":  (5,  6,  7,  8),
    "Middle": (9,  10, 11, 12),
    "Ring":   (13, 14, 15, 16),
    "Pinky":  (17, 18, 19, 20),
}

# Hand connections as index pairs (MediaPipe canonical topology)
HAND_CONNECTIONS = [
    (0,1),(1,2),(2,3),(3,4),         # thumb
    (0,5),(5,6),(6,7),(7,8),         # index
    (5,9),(9,10),(10,11),(11,12),    # middle
    (9,13),(13,14),(14,15),(15,16),  # ring
    (13,17),(17,18),(18,19),(19,20), # pinky
    (0,17),                          # palm
]
# ──────────────────────────────────────────────────────────────────────────────


class HandGestureLive:
    """Manages webcam capture, MediaPipe inference, and OpenCV rendering."""

    def __init__(self):
        self._result_lock = threading.Lock()
        self._latest_result: vision.HandLandmarkerResult | None = None

    # ── MediaPipe callback (called from internal thread) ──────────────────────
    def _on_result(
        self,
        result: vision.HandLandmarkerResult,
        output_image: mp.Image,
        timestamp_ms: int,
    ) -> None:
        with self._result_lock:
            self._latest_result = result

    def _get_latest_result(self) -> vision.HandLandmarkerResult | None:
        with self._result_lock:
            return self._latest_result

    # ── Finger flexion ─────────────────────────────────────────────────────────
    @staticmethod
    def _finger_flexion(hand_landmarks) -> dict[str, float]:
        """
        Returns flexion per finger as 0.0 (fully extended) – 100.0 (fist).
        Computed from the average of the angles at the PIP and DIP joints.
        Straight finger → ~180° angle → 0 %; fist → ~90° angle → 100 %.
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

        lm = hand_landmarks
        result = {}
        for name, (base, pip_i, dip_i, tip_i) in FINGER_JOINTS.items():
            if name == "Thumb":
                # Angle at MCP (lm[2]): between lm[1]-lm[2]-lm[3]
                mcp_angle = _angle(lm[base], lm[pip_i], lm[dip_i])
                # Angle at IP (lm[3]): between lm[2]-lm[3]-lm[4] — main flexion joint
                ip_angle  = _angle(lm[pip_i], lm[dip_i], lm[tip_i])
                avg_angle = (mcp_angle + ip_angle) / 2.0
                flex = max(0.0, min(100.0, (180.0 - avg_angle) / 90.0 * 100.0))
            else:
                pip_angle = _angle(lm[base], lm[pip_i], lm[dip_i])
                dip_angle = _angle(lm[pip_i], lm[dip_i], lm[tip_i])
                avg_angle = (pip_angle + dip_angle) / 2.0
                # 180° → 0 %, 90° → 100 % (clamped)
                flex = max(0.0, min(100.0, (180.0 - avg_angle) / 90.0 * 100.0))
            result[name] = round(flex, 1)
        return result

    # ── Drawing ───────────────────────────────────────────────────────────────
    @staticmethod
    def _draw_landmarks(bgr_frame: np.ndarray, result: vision.HandLandmarkerResult) -> np.ndarray:
        annotated = bgr_frame.copy()
        h, w = annotated.shape[:2]

        for hand_idx, (hand_landmarks, handedness) in enumerate(
            zip(result.hand_landmarks, result.handedness)
        ):
            # Pixel coordinates for each of the 21 landmarks
            pts = [
                (int(lm.x * w), int(lm.y * h))
                for lm in hand_landmarks
            ]

            # Draw connections
            for start_idx, end_idx in HAND_CONNECTIONS:
                cv2.line(annotated, pts[start_idx], pts[end_idx],
                         CONNECTION_COLOR, 2, cv2.LINE_AA)

            # Draw landmark dots
            for pt in pts:
                cv2.circle(annotated, pt, 5, LANDMARK_COLOR, -1, cv2.LINE_AA)
                cv2.circle(annotated, pt, 5, CONNECTION_COLOR, 1, cv2.LINE_AA)

            # Label (Left / Right) above the hand bounding box
            xs = [p[0] for p in pts]
            ys = [p[1] for p in pts]
            text_x = max(0, min(xs))
            text_y = max(MARGIN, min(ys) - MARGIN)
            label = handedness[0].category_name

            cv2.putText(
                annotated, label,
                (text_x, text_y),
                cv2.FONT_HERSHEY_DUPLEX,
                FONT_SIZE, HANDEDNESS_COLOR, FONT_THICKNESS, cv2.LINE_AA,
            )

            # ── Flexion HUD ───────────────────────────────────────────────────
            flexion = HandGestureLive._finger_flexion(hand_landmarks)
            panel_x = 10 + hand_idx * 220   # second hand panel offset
            panel_y = h - 10 - len(flexion) * 28
            bar_w, bar_h = 120, 14

            for row, (fname, fval) in enumerate(flexion.items()):
                row_y = panel_y + row * 28
                # finger name
                cv2.putText(
                    annotated, f"{fname[:3]}",
                    (panel_x, row_y + bar_h),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5,
                    HANDEDNESS_COLOR, 1, cv2.LINE_AA,
                )
                # background bar
                bx = panel_x + 40
                cv2.rectangle(annotated,
                              (bx, row_y),
                              (bx + bar_w, row_y + bar_h),
                              (50, 50, 50), -1)
                # filled bar (green → red based on flexion)
                filled = int(bar_w * fval / 100.0)
                bar_color = (
                    int(fval * 2.55),        # B
                    int((100 - fval) * 2.55), # G
                    0,
                )
                if filled > 0:
                    cv2.rectangle(annotated,
                                  (bx, row_y),
                                  (bx + filled, row_y + bar_h),
                                  bar_color, -1)
                # percentage label
                cv2.putText(
                    annotated, f"{fval:5.1f}%",
                    (bx + bar_w + 4, row_y + bar_h),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.45,
                    CONNECTION_COLOR, 1, cv2.LINE_AA,
                )

        return annotated

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

        with vision.HandLandmarker.create_from_options(options) as detector:
            print(f"Hand gesture detection running (camera {camera_index}) — press 'q' to quit.")
            while True:
                ret, bgr_frame = cap.read()
                if not ret:
                    print("Warning: failed to read frame, retrying…")
                    continue

                # Monotonically increasing timestamp required by LIVE_STREAM mode
                timestamp_ms = int((time.time() - start_time) * 1000)

                # MediaPipe expects RGB
                rgb_frame = cv2.cvtColor(bgr_frame, cv2.COLOR_BGR2RGB)
                mp_image = mp.Image(image_format=mp.ImageFormat.SRGB, data=rgb_frame)
                detector.detect_async(mp_image, timestamp_ms)

                # Render latest available result (may be one frame behind — acceptable)
                result = self._get_latest_result()
                display = self._draw_landmarks(bgr_frame, result) if result else bgr_frame

                # HUD: FPS approximation
                elapsed = time.time() - start_time
                fps = timestamp_ms / elapsed / 1000 if elapsed > 0 else 0
                cv2.putText(
                    display, f"FPS: {fps:.1f}",
                    (10, 30), cv2.FONT_HERSHEY_SIMPLEX,
                    0.8, (0, 255, 0), 2, cv2.LINE_AA,
                )

                cv2.imshow("Hand Gesture Live", display)
                if cv2.waitKey(1) & 0xFF == ord("q"):
                    break

        cap.release()
        cv2.destroyAllWindows()
        print("Stopped.")


def _list_cameras(max_check: int = 8) -> list[int]:
    """Return indices of all cameras that OpenCV can open."""
    available = []
    for i in range(max_check):
        cap = cv2.VideoCapture(i)
        if cap.isOpened():
            available.append(i)
            cap.release()
    return available


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Hand gesture live detection")
    parser.add_argument(
        "--camera", "-c",
        type=int,
        default=None,
        help="Camera index to use (default: auto-select or 0)",
    )
    parser.add_argument(
        "--list-cameras", "-l",
        action="store_true",
        help="List available camera indices and exit",
    )
    args = parser.parse_args()

    if args.list_cameras:
        cams = _list_cameras()
        if cams:
            print("Available cameras:", ", ".join(str(c) for c in cams))
        else:
            print("No cameras found.")
    else:
        cam_idx = args.camera if args.camera is not None else CAMERA_INDEX
        HandGestureLive().run(camera_index=cam_idx)
