#include <enableit.h>
#include "OtaCommandHandler.h"
#include <CommandParser.h>

OtaCommandHandler::OtaCommandHandler(String otaAppName, const char* uuid, uint32_t properties)
    : BleV1CommandDispatcher(uuid, properties), parser_(), otaAppName_(otaAppName)
{
    log_d("[OtaCommandHandler] ctor");
    parser_.init(this);
    parser_.add("OTA_STATUS", "Get OTA status", &OtaCommandHandler::cmdStatus);
    parser_.add("OTA_START", "Start OTA process: OTA_START <ssid>;<pass>", &OtaCommandHandler::cmdStart);
    parser_.add("OTA_STOP", "Stop OTA process", &OtaCommandHandler::cmdStop);
    parser_.add("OTA_URL", "Get OTA URL", &OtaCommandHandler::cmdUrl);
}

OtaCommandHandler::~OtaCommandHandler()
{
    log_d("[OtaCommandHandler] dtor");
}

void OtaCommandHandler::handle(const String &cmd, String &response)
{
    log_d("[OtaCommandHandler] handle");
    response.clear();
    currentResponse_.clear();
    parser_.parseLine(const_cast<char *>(cmd.c_str()));
    parser_.execute();
    response = currentResponse_;
}

// --- Command implementations ---

void OtaCommandHandler::cmdStatus()
{
    log_d("[OtaCommandHandler] cmdStatus");
    enableit::BoardApp *current = enableit::BoardManager::instance().getCurrentApp();
    String currentAppName = current ? current->name() : "";
    if (otaAppName_.isEmpty())
    {
        currentResponse_ = "ERROR_NO_OTA_APP_NAME";
        return;
    }
    if (currentAppName == otaAppName_)
    {
        currentResponse_ = "OTA_STATUS RUNNING";
        String url = current ? current->getInfo("url") : "";
        if (!url.isEmpty())
            currentResponse_ += "\nOTA_URL " + url;
    }
    else
    {
        currentResponse_ = "OTA_STATUS STOPPED";
    }
}

void OtaCommandHandler::cmdStart()
{
    log_d("[OtaCommandHandler] cmdStart");
    enableit::BoardApp *current = enableit::BoardManager::instance().getCurrentApp();
    String currentAppName = current ? current->name() : "";
    if (otaAppName_.isEmpty())
    {
        currentResponse_ = "ERROR_NO_OTA_APP_NAME";
        return;
    }
    if (currentAppName == otaAppName_)
    {
        currentResponse_ = "ALREADY_RUNNING";
        return;
    }

    String ssid, pass;
    if (parser_.getArgs() < 2)
    {
        currentResponse_ = "ERROR_MISSING_PARAMS";
        return;
    }

    auto otaapp = enableit::BoardManager::instance().getAppByName(otaAppName_.c_str());
    if (otaapp)
    {
        if (!otaapp->hasCapability(enableit::capabilities::Notify))
        {
            currentResponse_ = "ERROR_NOT_NOTIFIABLE_APP";
            return;
        }
    }
    else
    {
        currentResponse_ = "ERROR_OTA_APP_NOT_FOUND";
        return;
    }

    String arg = parser_.getString(1);
    int sep = arg.indexOf(';');
    if (sep < 0)
    {
        currentResponse_ = "ERROR_MISSING_PARAMS";
        return;
    }
    ssid = arg.substring(0, sep);
    pass = arg.substring(sep + 1);

    previousAppName_ = currentAppName;
    otaapp->setInfo("ssid", ssid);
    otaapp->setInfo("pass", pass);

    otaapp->setNotifyFn([this](const String &msg)
    {
        log_d("[OtaCommandHandler] notifyFn: %s", msg.c_str());
        size_t size = msg.length();
        const char* data = msg.c_str();
        this->notify((const uint8_t*)data, size); 
    });

    enableit::BoardManager::instance().setApp(otaAppName_.c_str());
    currentResponse_ = "OK";
}

void OtaCommandHandler::cmdStop()
{
    log_d("[OtaCommandHandler] cmdStop");
    enableit::BoardApp *current = enableit::BoardManager::instance().getCurrentApp();
    String currentAppName = current ? current->name() : "";
    if (currentAppName != otaAppName_)
    {
        currentResponse_ = "ERROR_NOT_OTA_APP";
        return;
    }
    if (!previousAppName_.isEmpty())
    {
        enableit::BoardManager::instance().setApp(previousAppName_.c_str());
    }
    else
    {
        enableit::BoardManager::instance().panic(0, "Stopping OTA app with no previous app");
    }
    previousAppName_.clear();
    currentResponse_ = "OK";
}

void OtaCommandHandler::cmdUrl()
{
    log_d("[OtaCommandHandler] cmdUrl");
    enableit::BoardApp *current = enableit::BoardManager::instance().getCurrentApp();
    String currentAppName = current ? current->name() : "";
    if (currentAppName != otaAppName_)
    {
        currentResponse_ = "ERROR_NOT_OTA_APP";
        return;
    }
    String url = current ? current->getInfo("url") : "";
    if (!url.isEmpty())
        currentResponse_ = "OTA_URL " + url;
    else
        currentResponse_ = "ERROR_NO_URL";
}
