// Compile-time test: log.h must work when a macro named ERROR already exists
// (as <windows.h> defines it) and must leave that macro unchanged.

#define ERROR 0   // same definition as in wingdi.h
#include "log.h"

static_assert(ERROR == 0, "log.h must restore the client's ERROR macro");

int main() {
    Logger logger{"macro"};
    logger(WARNING) << "log.h compiles with ERROR already defined";
    return 0;
}
