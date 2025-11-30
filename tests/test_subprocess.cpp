#include <gtest/gtest.h>
#include "subprocess-cpp.h"
#include <string>
#include <vector>

// Test basic subprocess invocation with default fnIsRunning (null)
TEST(SubprocessCppTest, BasicInvocationWithDefaultFnIsRunning) {
    const char *cmd[] = {"/bin/echo", "hello", nullptr};
    std::vector<std::string> outputLines;
    
    SubprocessCpp proc(cmd,
        [&outputLines](const char *str, bool isStdout) -> bool {
            outputLines.push_back(str);
            return true;
        });
    
    proc.stop();
    
    ASSERT_FALSE(outputLines.empty());
    EXPECT_NE(outputLines[0].find("hello"), std::string::npos);
}

// Test subprocess invocation with custom fnIsRunning
TEST(SubprocessCppTest, InvocationWithCustomFnIsRunning) {
    const char *cmd[] = {"/bin/echo", "ready", nullptr};
    bool gotReady = false;
    
    SubprocessCpp proc(cmd, nullptr,
        [&gotReady](const char *line) -> bool {
            if (std::string(line).find("ready") != std::string::npos) {
                gotReady = true;
                return true;
            }
            return false;
        });
    
    proc.stop();
    
    EXPECT_TRUE(gotReady);
}

// Test fnOnOutput callback receives output
TEST(SubprocessCppTest, FnOnOutputReceivesOutput) {
    const char *cmd[] = {"/bin/echo", "test output", nullptr};
    std::vector<std::string> capturedOutput;
    std::vector<bool> isStdoutFlags;
    
    SubprocessCpp proc(cmd,
        [&capturedOutput, &isStdoutFlags](const char *str, bool isStdout) -> bool {
            capturedOutput.push_back(str);
            isStdoutFlags.push_back(isStdout);
            return true;
        });
    
    proc.stop();
    
    ASSERT_FALSE(capturedOutput.empty());
    EXPECT_TRUE(isStdoutFlags[0]); // echo outputs to stdout
}

// Test subprocess with multiple output lines
TEST(SubprocessCppTest, MultipleOutputLines) {
    const char *cmd[] = {"/bin/sh", "-c", "echo line1; echo line2; echo line3", nullptr};
    std::vector<std::string> outputLines;
    
    SubprocessCpp proc(cmd,
        [&outputLines](const char *str, bool isStdout) -> bool {
            outputLines.push_back(str);
            return true;
        });
    
    proc.stop();
    
    EXPECT_GE(outputLines.size(), 3u);
}

// Test LogDestination_Screen (just ensure it doesn't crash)
TEST(SubprocessCppTest, LogDestinationScreen) {
    const char *cmd[] = {"/bin/echo", "screen test", nullptr};
    
    SubprocessCpp proc(cmd, nullptr, nullptr, SubprocessCpp::LogDestination_Screen);
    proc.stop();
    
    SUCCEED();
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
