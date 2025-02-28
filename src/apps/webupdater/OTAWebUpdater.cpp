#include <Arduino.h>
#include <Console.h>

//#include <WebServer.h>
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <FS.h>
#include <SPIFFS.h>
#include <Update.h>
#include <Config.h>

#include "OTAWebUpdaterApp.h"

#include "edit_html.h"
#include "manager_html.h"
#include "ok_html.h"
#include "failed_html.h"

const char* host = "enableit-filemanager";

String allowedExtensionsForEdit = "txt, h, htm, html, css, cpp, js";

const char* jquery = "/jquery-3.6.3.min.js";

//WebServer server(80);
AsyncWebServer server(80);

String filesDropdownOptions = "";
String textareaContent = "";
String savePath = "";
String savePathInput = "";

const char* param_delete_path = "delete_path";
const char* param_edit_path = "edit_path";
const char* param_edit_textarea = "edit_textarea";
const char* param_save_path = "save_path";

bool rebooting = false;

// misc functions
String convertFileSize(const size_t bytes)
{
  if(bytes < 1024)
  {
    return String(bytes) + " B";
  }
  else if (bytes < 1048576)
  {
    return String(bytes / 1024.0) + " kB";
  }
  else // if (bytes < 1073741824) // additional check not needed, won't update more than MB
  {
    return String(bytes / 1048576.0) + " MB";
  }
}

void notFound(AsyncWebServerRequest *request) 
{
  request->redirect("/manager");
  //request->send(404, "text/plain", "Page not found");
}


String listDir(fs::FS &fs, const char * dirname, uint8_t levels)
{
  filesDropdownOptions = "";
  String listenFiles = "<table><tr><th id=\"first_td_th\">List the library: </th><th>";
  listenFiles += dirname;
  listenFiles += "</th></tr>";

  File root = fs.open(dirname);
  String fail = "";
  if(!root)
  {
    fail = " the library cannot be opened";
    return fail;
  }
  if(!root.isDirectory())
  {
    fail = " this is not a library";
    return fail;
  }

  File file = root.openNextFile();
  while(file)
  {
    if(file.isDirectory())
    {
      listenFiles += "<tr><td id=\"first_td_th\">Library: ";
      listenFiles += file.name();

      filesDropdownOptions += "<option value=\"";
      filesDropdownOptions += file.name();
      filesDropdownOptions += "\">";
      filesDropdownOptions += file.name();
      filesDropdownOptions += "</option>";

      listenFiles += "</td><td> - </td></tr>";

      if(levels)
      {
        listDir(fs, file.name(), levels -1);
      }
    }
    else
    {
      listenFiles += "<tr><td id=\"first_td_th\">File: ";
      listenFiles += file.name();

      filesDropdownOptions += "<option value=\"";
      filesDropdownOptions += file.name();
      filesDropdownOptions += "\">";
      filesDropdownOptions += file.name();
      filesDropdownOptions += "</option>";

      listenFiles += " </td><td>\tSize: ";
      listenFiles += convertFileSize(file.size());
      listenFiles += "</td></tr>";
    }
    file = root.openNextFile();
  }
  listenFiles += "</table>";
  return listenFiles;  
}

String readFile(fs::FS &fs, const char * path)
{
  String fileContent = "";
  File file = fs.open(path, "r");
  if(!file || file.isDirectory())
  {
    return fileContent;
  }
  while(file.available())
  {
    fileContent+=String((char)file.read());
  }
  file.close();
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message)
{
  File file = fs.open(path, "w");
  if(!file)
  {
    return;
  }
  file.print(message);
  file.close();
}

void uploadFile(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) 
{
  if(!index)
  {
    request->_tempFile = SPIFFS.open("/" + filename, "w");
  }
  if(len)
  {
    request->_tempFile.write(data, len);
  }
  if(final)
  {
    request->_tempFile.close();
    request->redirect("/manager");
  }
}

String processor(const String& var)
{
  if(var == "ALLOWED_EXTENSIONS_EDIT")
  {
    return allowedExtensionsForEdit;
  }
  if(var == "SPIFFS_FREE_BYTES")
  {
    return convertFileSize((SPIFFS.totalBytes() - SPIFFS.usedBytes()));
  }

  if(var == "SPIFFS_USED_BYTES")
  {
    return convertFileSize(SPIFFS.usedBytes());
  }

  if(var == "SPIFFS_TOTAL_BYTES")
  {
    return convertFileSize(SPIFFS.totalBytes());
  }
  
  if(var == "LISTEN_FILES")
  {
    return listDir(SPIFFS, "/", 0);
  }

  if(var == "EDIT_FILES")
  {
    String editDropdown = "<select name=\"edit_path\" id=\"edit_path\">";
    editDropdown += "<option value=\"choose\">Select file to edit</option>";
    editDropdown += "<option value=\"new\">New text file</option>";
    editDropdown += filesDropdownOptions;      
    editDropdown += "</select>";
    return editDropdown;
  }
  
  if(var == "DELETE_FILES")
  {
    String deleteDropdown = "<select name=\"delete_path\" id=\"delete_path\">";
    deleteDropdown += "<option value=\"choose\">Select file to delete</option>";
    deleteDropdown += filesDropdownOptions;      
    deleteDropdown += "</select>";
    return deleteDropdown;
  }

  if(var == "TEXTAREA_CONTENT")
  {
    return textareaContent;
  }
  
  if(var == "SAVE_PATH_INPUT")
  {
    if(savePath == "/new.txt")
    {
      savePathInput = "<input type=\"text\" id=\"save_path\" name=\"save_path\" value=\"" + savePath + "\" >";
    }
    else
    {
      savePathInput = "";
    }
    return savePathInput;
  }
  return String();
}

/*
 * Login page
 */



/*
 * Server Index Page
 */



void OTAWebUpdater::enter(void) {
    LOG("Activating OTA Web Update");

    server.on("/manager", HTTP_GET, [](AsyncWebServerRequest *request)
      {
        if(!request->authenticate(config.httpUsername.c_str(), config.httpPassword.c_str()))
        {
          return request->requestAuthentication();
        }
        request->send_P(200, "text/html", manager_html, processor);
      });

    
      server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request)
      {
        rebooting = !Update.hasError();
        AsyncWebServerResponse *response = request->beginResponse(200, "text/html", rebooting ? ok_html : failed_html);

        response->addHeader("Connection", "close");
        request->send(response);
      },
      [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
      {
        if(!index)
        {
          OUT("Update firmware with image: %s\n", filename.c_str());
          OUTNOLF("Progress:");
//          OUTNOLF("Updating: ");
//          OUT(filename.c_str());

          if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
          {
            Update.printError(Serial);
          }
//          if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
//            Update.printError(Serial);
//          }

        }
        if(!Update.hasError())
        {
          if(Update.write(data, len) != len)
          {
            Update.printError(Serial);
          } else {
            OUTNOLF("*");
          }
        }
        if (final)
        {
          if(Update.end(true))
          {
            OUT("");
            OUTNOLF("Update completed, data written: ");
            OUT(convertFileSize(index + len).c_str());
            OUT("Rebooting...");
          }
          else
          {
            Update.printError(Serial);
          }
        }
      });


      server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) 
      {
        request->send(200);
      }, uploadFile);


      server.on("/edit", HTTP_GET, [](AsyncWebServerRequest *request)
      {
        if(!request->authenticate(config.httpUsername.c_str(), config.httpPassword.c_str()))
        {
          return request->requestAuthentication();
        }
        String inputMessage = request->getParam(param_edit_path)->value();
        if(inputMessage =="new")
        {
          textareaContent = "";
          savePath = "/new.txt";
        }
        else
        {
          savePath = inputMessage;
          textareaContent = readFile(SPIFFS, inputMessage.c_str());
        }
        request->send_P(200, "text/html", edit_html, processor);
      });


      server.on("/save", HTTP_GET, [](AsyncWebServerRequest *request)
      {
        if(!request->authenticate(config.httpUsername.c_str(), config.httpPassword.c_str()))
        {
          return request->requestAuthentication();
        }
        String inputMessage = "";
        if (request->hasParam(param_edit_textarea)) 
        {
          inputMessage = request->getParam(param_edit_textarea)->value();
        }
        if (request->hasParam(param_save_path)) 
        {
          savePath = request->getParam(param_save_path)->value();
        }
        writeFile(SPIFFS, savePath.c_str(), inputMessage.c_str());

        request->redirect("/manager");
      });


      server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request)
      {
        if(!request->authenticate(config.httpUsername.c_str(), config.httpPassword.c_str()))
        {
          return request->requestAuthentication();
        }
        String inputMessage = request->getParam(param_delete_path)->value();
        if(inputMessage !="choose")
        {
          SPIFFS.remove(inputMessage.c_str());
        }
        request->redirect("/manager");
      });


      server.on("/format", HTTP_POST, [](AsyncWebServerRequest *request)
      {
        SPIFFS.format();
        request->send(200);
        ESP.restart();
      });

      server.onNotFound(notFound);
      server.begin();

/*
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", loginIndex);
  });
  server.on("/upload", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });

  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      OUT("Update firmware with image: %s\n", upload.filename.c_str());
      OUTNOLF("Progress:");
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      } else {
        OUTNOLF("*");
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        OUT("");
        OUT("Update Success: %u", upload.totalSize);
        OUT("Rebooting...");
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();
*/
}

void OTAWebUpdater::process(void) {
  //server.handleClient();
  if(rebooting)
  {
    delay(100);
    ESP.restart();
  }
  delay(1);
}