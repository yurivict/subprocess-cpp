#include "subprocess-cpp.h"

#include <cstdlib>
#include <functional>
#include <iomanip>
#include <iostream>
#include <semaphore.h>
#include <sstream>
#include <thread>
#include <vector>
#include <unistd.h>
#include <sys/ioctl.h>

SubprocessCpp::SubprocessCpp(const char **command, std::function<bool(const char *str, bool isStdout)> fnOnOutput, std::function<bool(const char *)> fnIsRunning, LogDestination logDestination, const char *logFilePath) {
	// helpers
	auto timestamp = []() -> std::string {
		std::time_t currentTime = std::time(nullptr);
		std::tm* localTime = std::localtime(&currentTime);
		std::ostringstream ss;
		ss << std::put_time(localTime, "%Y-%m-%d_%H:%M:%S");
		return ss.str();
	};

	auto logProcessOutput = [logDestination,logFilePath,this,timestamp](const char *txt, bool isStdout) {
		if (logDestination != LogDestination_Hide) {
			std::ostringstream msg;
			msg << timestamp() << " server(" << (isStdout ? "stdout" : "stderr") << ") " << txt;
			if (logDestination == LogDestination_Screen)
				std::cout << msg.str();
			else if (logDestination == LogDestination_LogFile) {
				*this->serverLogFile << msg.str();
				this->serverLogFile->flush();
			}
		}
	};

	// form final command
	std::vector<const char*> command1 = {"/usr/bin/stdbuf", "-oL", "-eL"}; // line buffering for both stdout and stderr
	for (const char **p = command; *p != nullptr; ++p)
		command1.push_back(*p);
	command1.push_back(nullptr);

	// start subprocess
	int result = subprocess_create(&command1[0], subprocess_option_inherit_environment, this);
	if (result != 0) {
		std::cerr << "SubprocessCpp: failed to start subprocess: " << command[0] << std::endl;
		std::abort();
	}

	// openlog file
	serverLogFile = std::make_unique<std::ofstream>(logFilePath, std::ios::app);

	// semaphore to lock until process is running
	bool running = false;
	sem_t sem;
	sem_init(&sem, 0, 0); // semaphore starts locked

	// loop to read stdout until process is running, the start the watcher thread to keep watching stdout and stderr
	watcherThread = std::make_unique<std::thread>([this,logProcessOutput,fnIsRunning,fnOnOutput,&running,&sem]() {
		static const unsigned delta = 1024; // bytes to read at a time
		const unsigned timeout = 300000; // 0.3s usleep timeout
		struct StdX {
			unsigned idx;
			int fd;
			std::string buffer;
			bool eof;
			bool isStdout;
			unsigned skip;
			bool hasIncompleteLine() const {
				return !buffer.empty() && buffer.back() != '\n';
			}
		} stdx[2] = {
			{ 0, ::fileno(subprocess_stdout(this)), std::string(), false, true, 0 },
			{ 1, ::fileno(subprocess_stderr(this)), std::string(), false, false, 0 }
		};

		// monitor both stdout and stderr
		unsigned stopIterationsWithotOutput = 0;
		while (!stdx[0].eof || !stdx[1].eof) {
			::usleep(timeout);
			bool gotOutput = false;
			for (auto &sx : stdx) {
				int bytes = 0;
				while (!sx.eof && ::ioctl(sx.fd, FIONREAD, &bytes) == 0 && bytes > 0) { // TODO handle ioctl errors
					// read available data
					auto bufSize = sx.buffer.size();
					sx.buffer.resize(bufSize + delta);
					auto sz = ::read(sx.fd, (void*)(sx.buffer.c_str() + bufSize), delta);
					// TODO handle read errors
					if (sz == 0) {
						sx.eof = true;
						break;
					}
					sx.buffer.resize(bufSize + sz);
				}
				// iterate over complete lines
				auto eol = sx.buffer.find('\n');
				while (eol != std::string::npos) {
					// prevent cutting into incomplete lines of the other stream
					if (sx.skip < 5 && stdx[!sx.idx].hasIncompleteLine()) {
						sx.skip++;
						break;
					}
					sx.skip = 0;
					std::string line = sx.buffer.substr(0, eol + 1);
					logProcessOutput(line.c_str(), sx.isStdout);
					// call fnOnOutput callback if provided
					if (fnOnOutput)
						fnOnOutput(line.c_str(), sx.isStdout);
					// check if process is running
					if (!running) {
						bool isRunningNow = fnIsRunning ? fnIsRunning(line.c_str()) : true; // if fnIsRunning is null, consider running after any output
						if (isRunningNow) {
							running = true;
							sem_post(&sem);
						}
					}
					gotOutput = true;
					sx.buffer = sx.buffer.substr(eol + 1);
					eol = sx.buffer.find('\n');
				}
			}

			// handle the stopFlag
			if (this->stopFlag) {
				if (gotOutput) {
					stopIterationsWithotOutput = 0;
				} else {
					if (++stopIterationsWithotOutput >= 5)
						break; // enough iterations without output: exit
				}
			}
		}

		// drain the remaining buffers
		std::cout.flush();
	}); // end of thread code

	// wait until process is running
	sem_wait(&sem);
	sem_destroy(&sem);
}

void SubprocessCpp::stop() {
	if (watcherThread) {
		stopFlag = true;
		watcherThread->join();
	}
	subprocess_destroy(this);
}
