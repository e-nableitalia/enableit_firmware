#pragma once

namespace enableit {
namespace BleUuids {

constexpr char SERVICE[] = "00000000-8c9e-a95a-12bf-6e4c-3b9d-e0c2f1a8"; // Base service UUID

namespace Characteristic {
  constexpr char CONSOLE[]      = "00000100-8c9e-a95a-12bf-6e4c-3b9d-e0c2f1a8";
  constexpr char PROTOCOL[]     = "00000200-8c9e-a95a-12bf-6e4c-3b9d-e0c2f1a8";
  constexpr char OTA[]          = "00000300-8c9e-a95a-12bf-6e4c-3b9d-e0c2f1a8";
  
  constexpr char CONSOLE_RX[]   = "00010100-8c9e-a95a-12bf-6e4c-3b9d-e0c2f1a8";
  constexpr char CONSOLE_TX[]   = "00010200-8c9e-a95a-12bf-6e4c-3b9d-e0c2f1a8";
  constexpr char PROTOCOL_RX[]  = "00020100-8c9e-a95a-12bf-6e4c-3b9d-e0c2f1a8";
  constexpr char PROTOCOL_TX[]  = "00020200-8c9e-a95a-12bf-6e4c-3b9d-e0c2f1a8";
  constexpr char OTA_CTRL[]     = "00030100-8c9e-a95a-12bf-6e4c-3b9d-e0c2f1a8";
  constexpr char OTA_DATA[]     = "00030200-8c9e-a95a-12bf-6e4c-3b9d-e0c2f1a8";
}

} // namespace BleUuids
} // namespace enableit
