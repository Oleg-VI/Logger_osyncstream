# Logger_osyncstream

A tiny, thread-safe console logger for C++20, built on `std::osyncstream`.

Two files (`log.h` + `log.cpp`), about 150 lines in total, only the standard
library, and not a single mutex, lock, atomic or flag in the logger code itself.

```cpp
#include "log.h"

auto logger = getLogger("Main");
logger << "Starting the app";                          // INFO by default
logger(WARNING) << "Time spent: " << 10.5 << " s";

Logger worker{"Worker"};
worker(ERROR) << "Failed to open " << path;
```

```
29.09.2026 22:07:07; INFO; Main(10828): Starting the app
29.09.2026 22:07:07; WARNING; Main(10828): Time spent: 10.5 s
29.09.2026 22:07:07; ERROR; Worker(11660): Failed to open config.json
```

Format: `<Date> <Time>; <Level>; <Prefix>(<Thread Id>): <Message>`

## Features

- **Four fixed log levels:** `DEBUG`, `INFO`, `WARNING`, `ERROR`. The set is a
  closed `enum class`, so clients cannot add their own levels.
- **Custom prefix per logger**, added to every message automatically.
- **Safe for multithreaded use.** Every message reaches the console as one
  whole line. Lines from different threads never mix, never split and never
  produce blank lines.
- **Order within a thread is kept.** Messages from one thread appear in the
  same order as in the code.
- **Familiar `<<` syntax.** Anything that has an `operator<<` for
  `std::ostream` can be logged, including your own types.
- **Only the standard library.** No third-party libraries or build-time
  setup. Just add two files to your project.

## How it works

```
logger(DEBUG) << "value = " << 42;
└─────┬─────┘
      └─ returns a temporary LogMessage that owns a std::osyncstream
         ├─ constructor: writes "<timestamp>; DEBUG; prefix(tid): " into its private buffer
         ├─ operator<<:  appends to the same private buffer (no other thread can see it)
         └─ end of full expression → destructor:
               ~LogMessage()  appends '\n'
               ~osyncstream() calls emit() → the whole line goes to std::cout in one write
```

`std::osyncstream` (C++20, `<syncstream>`) gathers its output in a private
buffer. When it is destroyed, it moves the whole buffer to the target stream
in one step (`emit()`). While doing this it holds a mutex that the standard
library shares between all `osyncstream`s writing to the same target stream.
So the locking still happens, but inside the standard library, where it is
correct by construction. The logger code has none of its own.

## Design decisions

| Decision | Why |
|---|---|
| `std::osyncstream` as a member of `LogMessage` | The message is collected once, in one buffer, and written as a whole line. There is no `std::ostringstream` in between and the string is never copied. |
| One temporary per message, finished by its destructor (RAII) | A message ends exactly at the `;` of the logging statement. The client never calls `flush()` or `end()`. |
| `LogMessage` cannot be copied or moved | Both `Logger::operator()` and `Logger::operator<<` return a prvalue. From C++17 on, the compiler must build it straight in place (guaranteed copy elision). Without a move there is no moved-from object, so no "who owns the output" flag is needed. |
| `enum class LogLevel` + `using enum LogLevel` | The set of levels is closed and type-safe (no implicit conversion to `int`). The short names `DEBUG` and `INFO` work without writing each level twice. |
| `[[nodiscard]]` on `Logger::operator()` only | `logger(DEBUG);` on its own would print an empty message, so the compiler warns about it. The attribute is *not* on `LogMessage` or `Logger::operator<<`, because `logger << "text";` drops its result on purpose. |
| Header and source split | The header needs only `<string>`, `<string_view>` and `<syncstream>`. The time and thread helpers, and the headers they need (`<chrono>`, `<iomanip>`, `<thread>`, ...), stay in `log.cpp`. |
| Timestamp from `localtime_s` / `localtime_r`, not `std::chrono::current_zone()` | On Windows, libstdc++ (MinGW, at least GCC 13) cannot find the system time zone and quietly uses UTC. The C runtime conversion gives the right local time on every platform we tried. Both functions are thread-safe, unlike `std::localtime`. |
| Timestamp written straight into the message stream | `std::put_time` writes into the same `osyncstream`, so no temporary string is created. |

## Where it fits, including commercial code

The logger is a good choice when **the console is the only place logs go** and
you want something correct, small and easy to review:

- Command-line tools, utilities, installers, build and deployment helpers.
- Services running in containers, where stdout goes to Docker, Kubernetes or
  systemd-journald and the platform collects the logs.
- Test tools, benchmarks, simulations, prototypes and internal tools.
- Libraries or modules that must not add a third-party dependency (licensing,
  certification or supply-chain rules), but still need diagnostic output that
  is safe to write from several threads.
- A first step before a larger logging framework: the `logger(LEVEL) << ...`
  syntax is close to most frameworks, so switching later is simple.
- Teaching and code review: the whole design is visible at a glance.

## Limitations and when *not* to use it

Know these limits before you ship it. Each of them is a deliberate trade-off
for simplicity.

**Missing features**
- **Console only.** No files, rotation, syslog, network or several outputs at
  once.
- **No level filtering.** Every message is formatted and printed, including
  `DEBUG`. There is no minimum level to set at run time, and debug logging
  cannot be removed at compile time.
- **Plain text only.** No JSON or key/value fields for log collectors.
- **Fixed format.** The date format and field order are set in the code. The
  timestamp has one-second precision.

**Performance**
- **Logging is synchronous.** The calling thread writes to the console itself,
  inside `emit()`. A console is slow, especially the Windows console, so
  under heavy load threads will wait for each other on the shared `std::cout`
  lock. If you log in a hot path, or in latency-critical code such as
  real-time, audio, trading or game loops, use an asynchronous logger with a
  background thread (spdlog async, Quill, Boost.Log, ...).
- Every message creates its own stream buffer (a heap allocation) and reads
  the clock. This is fine for normal logging, but not for millions of messages
  per second.

**Correctness limits**
- Only other `osyncstream`s are synchronized with the logger. Plain
  `std::cout << ...`, `printf` or `std::cerr` from other code can still appear
  between log lines, although a log line itself is never split.
- Output is buffered until the statement ends. If the process crashes inside
  a logging statement, that message is lost. There is no crash-safe flush.
- Do not use it in signal handlers: it allocates memory and takes locks.
- Logging from the destructors of static or global objects after `main`
  returns depends on the standard library keeping `std::cout` alive. In
  practice it does, but it is not guaranteed for every static object.
- The thread ID format depends on the platform: large system IDs with MSVC,
  small sequential numbers with MinGW/winpthreads.
- On Windows, `<windows.h>` defines a macro named `ERROR`. Include `log.h`
  first, or `#undef ERROR` before including it.

**Rule of thumb:** if you need files, filtering, structured output or high
throughput, use a full logging framework. If you need correct, readable,
thread-safe console output with no dependencies, this is enough.

## Requirements

A C++20 compiler whose standard library supports `<syncstream>`:

- MSVC 19.29 (VS 2019 16.10) or newer
- GCC 11 or newer (libstdc++)
- Clang: with libstdc++ it works as above. libc++ added `<syncstream>` late,
  so check your version.

Tested with **MinGW-w64 GCC 13.1** and **MSVC (Visual Studio 2026)**. The tests
used the sample `main.cpp` and a stress test with 8 threads × 2000 messages,
random `yield`/`sleep`, and checks of the line format and of per-thread order.
Both builds had no warnings with `-Wall -Wextra` and `/W4`.

## Building

```bash
cmake -S . -B build
cmake --build build
```

Or add `log.h` and `log.cpp` to any C++20 project.
