#pragma once

#ifdef ENABLEIT_BOARD_BPI_LEAF_S3

#include <Arduino.h>
#include <Board.h>
#include <WifiHalBpiLeafS3.h>

class BpiLeafS3 : public Board {
public:
    BpiLeafS3();
    ~BpiLeafS3();

    void begin(bool lcdEnabled = true) override;
    void end() override;
    // Add display or other HAL as needed

    // HAL aggregate
    WifiHal& wifi() override { return wifi_; }

private:
    WifiHalBpiLeafS3 wifi_;
};

#endif
