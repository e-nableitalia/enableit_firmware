#pragma once

namespace BleUuids {

constexpr char VENDOR_BASE[] = "a8f1c2e0-9d3b-4c6e-bf12-5a9e8c000000";

namespace Console {
  constexpr char SERVICE[] = "a8f1c2e0-9d3b-4c6e-bf12-5a9e8c010000";
  constexpr char RX[]      = "a8f1c2e0-9d3b-4c6e-bf12-5a9e8c010001";
  constexpr char TX[]      = "a8f1c2e0-9d3b-4c6e-bf12-5a9e8c010002";
}

namespace Protocol {
  constexpr char SERVICE[] = "a8f1c2e0-9d3b-4c6e-bf12-5a9e8c020000";
  constexpr char RX[]      = "a8f1c2e0-9d3b-4c6e-bf12-5a9e8c020001";
  constexpr char TX[]      = "a8f1c2e0-9d3b-4c6e-bf12-5a9e8c020002";
}

namespace Ota {
  constexpr char SERVICE[] = "a8f1c2e0-9d3b-4c6e-bf12-5a9e8c030000";
  constexpr char CTRL[]    = "a8f1c2e0-9d3b-4c6e-bf12-5a9e8c030001";
  constexpr char DATA[]    = "a8f1c2e0-9d3b-4c6e-bf12-5a9e8c030002";
}

}
