// Stress test: several threads log at the same time with random yields and
// sleeps. Checks that every line is complete and well-formed, that there are
// no blank or split lines, and that the order within each thread is kept.

#include "log.h"

#include <chrono>
#include <iostream>
#include <map>
#include <random>
#include <regex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr int kThreads           = 8;
constexpr int kMessagesPerThread = 2000;

void worker(int id) {
    std::mt19937 rng{static_cast<unsigned>(id)};
    Logger logger{"T" + std::to_string(id)};

    for (int i = 0; i < kMessagesPerThread; ++i) {
        // Odd messages: default INFO level, several << and a variable-length tail.
        // Even messages: explicit WARNING level.
        if (i % 2)
            logger << "seq=" << i << " payload " << 3.5 << ' ' << std::string(rng() % 50, 'x');
        else
            logger(WARNING) << "seq=" << i;

        if (rng() % 7 == 0)
            std::this_thread::yield();
        if (rng() % 97 == 0)
            std::this_thread::sleep_for(std::chrono::microseconds{static_cast<int>(rng() % 500)});
    }
}

int fail(const std::string& reason) {
    std::cerr << "FAILED: " << reason << '\n';
    return 1;
}

} // namespace

int main() {
    // LogMessage wraps the stream buffer std::cout has at that moment,
    // so redirecting it captures all log output.
    std::stringbuf capture;
    std::streambuf* const original{std::cout.rdbuf(&capture)};
    {
        std::vector<std::jthread> threads;
        for (int t = 0; t < kThreads; ++t)
            threads.emplace_back(worker, t);
    }
    std::cout.rdbuf(original);

    const std::string output{capture.str()};
    if (output.empty() || output.back() != '\n')
        return fail("output must end with a newline");

    const std::regex lineRe{
        R"(^\d\d\.\d\d\.\d{4} \d\d:\d\d:\d\d; (INFO|WARNING); T(\d+)\(\d+\): seq=(\d+)( payload 3\.5 x*)?$)"};

    std::map<int, int> lastSeq;   // thread number -> last seen sequence number
    int count{0};

    std::istringstream in{output};
    for (std::string line; std::getline(in, line); ++count) {
        std::smatch m;
        if (!std::regex_match(line, m, lineRe))
            return fail("malformed line: \"" + line + '"');

        const bool isInfo{m[1] == "INFO"};
        const int  thread{std::stoi(m[2])};
        const int  seq{std::stoi(m[3])};

        if (isInfo != (seq % 2 == 1) || isInfo != m[4].matched)
            return fail("level does not match message: \"" + line + '"');

        const auto it = lastSeq.find(thread);
        const int expected{it == lastSeq.end() ? 0 : it->second + 1};
        if (seq != expected)
            return fail("thread T" + std::to_string(thread) + ": expected seq=" +
                        std::to_string(expected) + ", got seq=" + std::to_string(seq));
        lastSeq[thread] = seq;
    }

    if (count != kThreads * kMessagesPerThread)
        return fail("expected " + std::to_string(kThreads * kMessagesPerThread) +
                    " lines, got " + std::to_string(count));

    std::cout << "OK: " << count << " lines from " << kThreads << " threads\n";
    return 0;
}
