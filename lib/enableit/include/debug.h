//
// Console debug functions
//
// Author: A.Navatta / e-Nable Italia

#ifndef CONSOLE_DEBUG_H

#define CONSOLE_DEBUG_H

// debug enabled
#define DEBUG
#define FORMAT_BUFFERSIZE 1024

void debug_enable(bool enabled);
void console_debug(const bool dbg, const char *info, bool addcr, const char *function, const char *format_str, ...);

#ifdef DEBUG
#define DBG(...) console_debug(true, "DBG", true, __func__,  __VA_ARGS__)
#define DBGNOLF(...) console_debug(true, "DBG", false, __func__,  __VA_ARGS__)
#define LOG(...) console_debug(true, "LOG", true, __func__,  __VA_ARGS__)
#define ERR(...) console_debug(false, "ERR", true, __func__,  __VA_ARGS__)
#define OUTNOLF(...) console_debug(true, nullptr, false, __func__,  __VA_ARGS__)
#define OUT(...) console_debug(false, nullptr, true, nullptr,  __VA_ARGS__)
#else
#define DBG(...)
#define DBGNOLF(...)
#define LOG(...) console_debug(true, "LOG", true, __func__,  __VA_ARGS__)
#define ERR(...) console_debug(false, "ERR", true, __func__,  __VA_ARGS__)
#define OUTNOLF(...) console_debug(true, nullptr, false, __func__,  __VA_ARGS__)
#define OUT(...) console_debug(false, nullptr, true, nullptr,  __VA_ARGS__)
#endif

#endif // CONSOLE_DEBUG_H