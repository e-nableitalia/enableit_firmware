#pragma once
#include <BleCommandDispatcher.h>
#include <SystemInfoProvider.h>
#include <enableit.h>

#define SYSTEM_CHARACTERISTIC_UUID "b2a49d41-a2ac-48c3-b6c8-cfd05640654e"

class SystemConfigCommandDispatcher : public enableit::BleV1CommandDispatcher {
public:
    SystemConfigCommandDispatcher();
    const char* name() const override { return "systemConfig"; }
    void handle(const String& cmd, String& response) override;
};
