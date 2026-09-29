#include <thread>
#include "log.h"

/*
Example of console output:
23.02.2020 18:06:06; INFO; (1020): Starting the app
23.02.2020 18:06:06; DEBUG; (2561): Running a thread
...
23.02.2020 18:06:06; INFO; (1020): Finishing the app
*/


void f1() {
    auto logger1 = getLogger();
    // output: 23.02.2020 18:06:06; DEBUG; (2561): Running a thread
    logger1(DEBUG) << "Running a thread";

    auto logger2 = getLogger("f1"); // Each message has f1 prefix
    // output: 23.02.2020 18:06:06; INFO; f1(2561): Running a thread
    logger2 << "Running a thread" << " but log to another logger"; // Default log level INFO
}

void f2() {
    Logger logger{"f2"}; // Each message has f2 prefix
    // output: 23.02.2020 18:06:06; DEBUG; f2(3444): Running a thread
    logger(DEBUG) << "Running a thread";

    const double timeSpent = 10.0;
    // output: 23.02.2020 18:06:06; WARNING; f2(3444): Time spent in the thread: 10.0 seconds
    logger(WARNING) << "Time spent in the thread: " << timeSpent << " seconds";
}

void f3() {
    auto logger = getLogger("f3");  // Each message has f3 prefix
    // output: 23.02.2020 18:06:06; INFO; f3(3444): Running a thread
    logger << "Running a thread"; // Default log level INFO

    const int i = 123;
    // output: 23.02.2020 18:06:06; ERROR; f3(3444): My int is 123
    logger(ERROR) << "My int is " << i;
    // output: 23.02.2020 18:06:06; INFO; f3(3444): This is a random message to test log levels
    logger << "This is a random message " << "to test log levels";
}

int main() {
    auto logger = getLogger();
    // output: 23.02.2020 18:06:06; INFO; (1020): Starting the app
    logger << "Starting the app"; // Default log level INFO

    logger(DEBUG) << "Running the threads";
    std::thread t1(f1);
    std::thread t2(f2);
    std::thread t3(f3);

    t1.join();
    t2.join();
    t3.join();

    logger << "Finishing the app"; // Default log level INFO
    return 0;
}
