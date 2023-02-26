//
// CommandParser: Simple template command line commands parser
//
// Author: A.Navatta / e-Nable Italia

#ifndef COMMAND_PARSER_H

#define COMMAND_PARSER_H

#include <Arduino.h>
#include <debug.h>

#define BUFFER_MAX      256
#define MAX_ARGS        10
#define MAX_COMMANDS    20

#define PROMPT          "Command: "
#define PROMPT_CMDERROR "Unknown command <%s>"

template <class TClass>
class CommandParser {
public:
    CommandParser();
    void init(TClass *i);
    void poll();
    void add(const char *command, const char *help, void (TClass::*fpt)());
    void display();
    int getInt(int pos);
    char *getString(int pos);
    bool getBool(int pos);
    const char *getCommand(int i);
    const char *getHelp(int i);    
private:
    void parse();

    char line[BUFFER_MAX];
    int row;
    char *argv[MAX_ARGS];
    uint8_t argc;

    bool echo;
    bool human;
    bool prompt;
    
    struct {
        const char *cmd;
        const char *help;
        void (TClass::*fpt)();
    } command_table[MAX_COMMANDS];
    TClass *ptrObj;
};

#include <CommandParser.inl>

#endif // COMMAND_PARSER_H