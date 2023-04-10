//
// CommandParser: Simple template command line commands parser, inline functions
//
// Author: A.Navatta / e-Nable Italia

#include <USB.h>
#include <debug.h>
#include <Console.h>
#include <CommandParser.h>

template <class TClass>
CommandParser<TClass>::CommandParser() {
    human = echo = prompt = true;
    // command input
    row = 0;
    memset(line,0,BUFFER_MAX);

    for (int i = 0; i < MAX_COMMANDS; i++) {
        command_table[i].cmd = nullptr;
        command_table[i].help = nullptr;
        command_table[i].fpt = nullptr;
    }
}

template <class TClass>
void CommandParser<TClass>::init(TClass *i) {
    ptrObj = i;
}

template <class TClass>
void CommandParser<TClass>::add(const char *command, const char *h, void (TClass::*fpt)()) {
       for (int i = 0; i < MAX_COMMANDS; i++) {
        if (command_table[i].cmd == nullptr) {
            command_table[i].cmd = command;
            command_table[i].help = h;
            command_table[i].fpt = fpt;
            break;
        }
    }
}

template <class TClass>
void CommandParser<TClass>::parse() {
    uint8_t i = argc =  0;

    if (strlen(line) == 0)
        return;

    argv[i] = strtok(line, " ");
    
    do {
        argv[++i] = strtok(NULL, " ");
    } while ((i < 30) && (argv[i] != NULL));

    argc = i;

    // execute command
    for (int i = 0; i < MAX_COMMANDS; i++) {
        if ((command_table[i].cmd!= nullptr) && (!strcasecmp(command_table[i].cmd,argv[0]))) {
            // execute command
            (*ptrObj.*(command_table[i].fpt))();
            return;
        }
    }

    if (argv[0])
        OUT(PROMPT_CMDERROR, argv[0]);
    else
        OUT(PROMPT_CMDERROR, "");
}

template <class TClass>
int CommandParser<TClass>::getInt(int pos) {
    if (pos > argc)
        return 0;
    
    return atoi(argv[pos]);
}

template <class TClass>
const char *CommandParser<TClass>::getString(int pos) {
    if (pos > argc)
        return "";
    
    return argv[pos];
}

template <class TClass>
bool CommandParser<TClass>::getBool(int pos) {
        if (pos > argc)
            return false;
        
        return !strcmp(argv[pos],"true");
}

template <class TClass>
void CommandParser<TClass>::poll() {
    if (prompt)
        display();

    // do serial input
    while (Console.available() > 0) {
        // read the incoming byte
        char readchar = Console.read();
        // check end of line
        if (readchar == '\r') {
            // process line feed
            Console.println("");
            // force command processing
            parse();
            prompt = true;            
        } else if (readchar == '\n') {
            // just skip carriage return
//            DBGSerial.println("");
            // force command processing
//            parse();
            break;
        } else if ((readchar == '\b') && (row > 0 )) {
            // process backspace
            line[--row] = 0;
            // hack, clear char
            Console.print("\b \b");
        } else {
            // do char echo, if echo enabled
            if (echo)
                Console.print(readchar);
            else
                Console.print('*');

            // queue char in buffer
            line[row++] = readchar;
            // leave if buffer overflow
            if (row >= BUFFER_MAX) {
                // force command processing
                ERR("Serial buffer overflow");
                display();
                break;
            }
        }
    }
}

template <class TClass>
void CommandParser<TClass>::display() {
    Console.print(PROMPT);
    row = 0;
    memset(line,0,BUFFER_MAX);
    prompt = false;
}

template <class TClass>
const char *CommandParser<TClass>::getCommand(int i) {
    return command_table[i].cmd;
}

template <class TClass>
const char *CommandParser<TClass>::getHelp(int i) {
    return command_table[i].help;
}
