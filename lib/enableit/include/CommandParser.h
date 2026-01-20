//
// CommandParser: Simple template command line commands parser
//
// Author: A.Navatta / e-Nable Italia
#pragma once

#include <Arduino.h>
#include <Console.h>

#define BUFFER_MAX      256
#define MAX_ARGS        10
#define MAX_COMMANDS    30

#define PROMPT          "Command: "
#define PROMPT_CMDERROR "Unknown command <%s>"

template <class TClass>
class CommandParser {
public:
    CommandParser();
    void init(TClass *i);
    void add(const char *command, const char *help, void (TClass::*fpt)());
    virtual void parseLine(char *buffer);
    virtual bool execute();
    int getArgs() { return argc; }
    int getInt(int pos);
    const char *getString(int pos);
    bool getBool(int pos);
    const char *getCommand();
    const char *getHelp(int i);
protected:
    void parse();
    char line[BUFFER_MAX];
    char *argv[MAX_ARGS];
    uint8_t argc;
    struct {
        const char *cmd;
        const char *help;
        void (TClass::*fpt)();
    } command_table[MAX_COMMANDS];
    TClass *ptrObj;
    int commandIndex; // index of matched command, -1 if none
};

template <class TClass>
class ConsoleCommandParser : public CommandParser<TClass> {
public:
    ConsoleCommandParser();
    virtual void parseLine(char *buffer) override;
    virtual bool execute() override;
    void poll();
    void display();
private:
    int row;
    bool human;
    bool prompt;
};

#include <CommandParser.inl>
// --- CommandParser inline functions ---