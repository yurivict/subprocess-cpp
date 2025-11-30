#pragma once

//
// This module extends the functionality of the 'subprocess.h' library by providing
// * C++ classes that encapsulate the subprocess structure and its operations.
// * only continue when user-defined function indicates that the subprocess is already running
// * subprocess stdin/stdout watcher thread that saves its log to a file.
//

#include "subprocess.h"

#include <fstream>
#include <functional>
#include <memory>
#include <thread>

class SubprocessCpp : protected subprocess_s {
    std::unique_ptr<std::thread>    watcherThread;
    std::unique_ptr<std::ofstream>  serverLogFile;
    bool                            stopFlag = false;

public: // types
    enum LogDestination {
        LogDestination_Hide,
        LogDestination_Screen,
        LogDestination_LogFile
    };

public:
    SubprocessCpp(
        const char **command,
        std::function<bool(const char *str, bool isStdout)> fnOnOutput = nullptr, // optional callback for each output line
        std::function<bool(const char *)> fnIsRunning = nullptr, // condition when to continue once subprocess started (optional: if null, consider running after any output)
        LogDestination logDestination = LogDestination_Hide,
        const char *logFilePath = nullptr);

    void stop();
};
