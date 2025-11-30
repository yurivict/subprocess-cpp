# subprocess-cpp

subprocess-cpp is a simple subprocess library for C++.

## Description

- Allows you to start a child process easily.
- Allows you to monitor or capture its standard output and error streams.
- Allows you to save output into the log file.
- It is based on the C library [subprocess.h](https://github.com/sheredom/subprocess.h).

## Usage Example

```cpp
#include "subprocess-cpp.h"
#include <iostream>
#include <string>

int main() {
    // Basic usage: run a command and wait for any output
    const char *cmd[] = {"/bin/echo", "Hello, World!", nullptr};
    SubprocessCpp proc(cmd);
    proc.stop();

    // With output callback
    const char *cmd2[] = {"/bin/sh", "-c", "echo line1; echo line2", nullptr};
    SubprocessCpp proc2(cmd2,
        [](const char *str, bool isStdout) -> bool {
            std::cout << (isStdout ? "stdout: " : "stderr: ") << str;
            return true;
        });
    proc2.stop();

    // With custom "is running" condition
    const char *cmd3[] = {"/usr/bin/some-server", nullptr};
    SubprocessCpp proc3(cmd3,
        [](const char *str, bool isStdout) -> bool {
            std::cout << str;
            return true;
        },
        [](const char *line) -> bool {
            // Continue once server reports it's ready
            return std::string(line).find("Server started") != std::string::npos;
        });
    // ... use the server ...
    proc3.stop();

    // Log output to screen
    const char *cmd4[] = {"/bin/echo", "logged output", nullptr};
    SubprocessCpp proc4(cmd4, nullptr, nullptr, SubprocessCpp::LogDestination_Screen);
    proc4.stop();

    // Log output to file
    const char *cmd5[] = {"/bin/echo", "file output", nullptr};
    SubprocessCpp proc5(cmd5, nullptr, nullptr,
        SubprocessCpp::LogDestination_LogFile, "/tmp/subprocess.log");
    proc5.stop();

    return 0;
}
```
