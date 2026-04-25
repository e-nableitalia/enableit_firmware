//
// CommandParser: Simple template command line commands parser, inline functions
//
// Author: A.Navatta / e-Nable Italia

#include <USB.h>
#include <Console.h>
#include <CommandParser.h>

using namespace enableit;

// --- CommandParser ---

template <class TClass>
CommandParser<TClass>::CommandParser() {
    argc = 0;
    memset(line, 0, BUFFER_MAX);
    for (int i = 0; i < MAX_COMMANDS; i++) {
        command_table[i].cmd = nullptr;
        command_table[i].help = nullptr;
        command_table[i].fpt = nullptr;
    }
    ptrObj = nullptr;
    commandIndex = -1;
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
void CommandParser<TClass>::parseLine(char *buffer) {
    strncpy(line, buffer, BUFFER_MAX - 1);
    line[BUFFER_MAX - 1] = 0;
    parse();
}

template <class TClass>
void CommandParser<TClass>::parse() {
    uint8_t i = argc = 0;
    commandIndex = -1;

    if (strlen(line) == 0)
        return;

    argv[i] = strtok(line, " ");
    do {
        argv[++i] = strtok(NULL, " ");
    } while ((i < MAX_ARGS - 1) && (argv[i] != NULL));

    argc = i ? i-1 : 0;

    // find command
    for (int j = 0; j < MAX_COMMANDS; j++) {
        if ((command_table[j].cmd != nullptr) && (!strcasecmp(command_table[j].cmd, argv[0]))) {
            commandIndex = j;
            return;
        }
    }
}

template <class TClass>
bool CommandParser<TClass>::execute() {
    if (commandIndex >= 0 && command_table[commandIndex].fpt) {
        (*ptrObj.*(command_table[commandIndex].fpt))();
        return true;
    }
    return false;
}

template <class TClass>
int CommandParser<TClass>::getInt(int pos) {
    if (pos >= argc)
        return 0;
    return atoi(argv[pos]);
}

template <class TClass>
const char *CommandParser<TClass>::getString(int pos) {
    if (pos >= argc)
        return "";
    return argv[pos];
}

template <class TClass>
bool CommandParser<TClass>::getBool(int pos) {
    if (pos >= argc)
        return false;
    return !strcmp(argv[pos], "true");
}

template <class TClass>
const char *CommandParser<TClass>::getCommand() {
    if (argc > 0)
        return argv[0];
    return "";
}

template <class TClass>
const char *CommandParser<TClass>::getHelp(int i) {
    return command_table[i].help;
}

// --- ConsoleCommandParser ---

template <class TClass>
ConsoleCommandParser<TClass>::ConsoleCommandParser()
    : CommandParser<TClass>() {
    human = prompt = true;
    row = 0;
}

template <class TClass>
void ConsoleCommandParser<TClass>::poll() {
    Console.poll();

    if (prompt)
        display();

    // do serial input
    while (Console.available() > 0) {
        char readchar = Console.read();
        if (readchar == '\r') {
            log_d("");
            this->line[row] = 0;
            this->parse();
            this->execute();
            prompt = true;
        } else if (readchar == '\n') {
            break;
        } else if ((readchar == '\b') && (row > 0 )) {
            this->line[--row] = 0;
            Console.print("\b \b");
        } else {
            Console.echo(readchar);
            this->line[row++] = readchar;
            if (row >= BUFFER_MAX) {
                log_e("Serial buffer overflow");
                display();
                break;
            }
        }
    }
}

template <class TClass>
void ConsoleCommandParser<TClass>::display() {
    Console.print(PROMPT);
    row = 0;
    memset(this->line, 0, BUFFER_MAX);
    prompt = false;
}

template <class TClass>
void ConsoleCommandParser<TClass>::parseLine(char *buffer) {
    log_d("Sending command:[%s]", buffer);
    while (*buffer) {
        Console.echo(*buffer);
        this->line[row++] = *buffer++;
        if (row >= BUFFER_MAX) {
            log_e("Serial buffer overflow");
            display();
            break;
        }
    }
    this->line[row] = 0;
    this->parse();
    this->execute();
}

template <class TClass>
bool ConsoleCommandParser<TClass>::execute() {
    if (!CommandParser<TClass>::execute()) {
        if (this->argv[0])
            OUT(PROMPT_CMDERROR, this->argv[0]);
        else
            OUT(PROMPT_CMDERROR, "");
        return false;
    }
    return true;
}

// --- End of CommandParser inline functions ---
