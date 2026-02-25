#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include "can_parser.h"

int testsPassed = 0;
int testsFailed = 0;

void printHeader(const std::string& testName) {
    std::cout << "\n--- " << testName << " ---\n";
}

void check(bool condition, std::string_view testName, std::string_view errorMsg = "Condition failed") {
    if (condition) {
        std::cout << "[PASS] " << testName << "\n";
        testsPassed++;
    } else {
        std::cerr << "[FAIL] " << testName << " - " << errorMsg << "\n";
        testsFailed++;
    }
}

void checkPayload(const std::vector<uint8_t>& actual, const std::vector<uint8_t>& expected, const std::string& testName) {
    if (actual.size() != expected.size()) {
        std::cerr << "[FAIL] " << testName << " - Length mismatch ("
                  << actual.size() << " vs " << expected.size() << ")\n";
        testsFailed++;
        return;
    }

    for (size_t i = 0; i < actual.size(); ++i) {
        if (actual[i] != expected[i]) {
            std::cerr << "[FAIL] " << testName << " - Byte mismatch at index " << i
                      << " (" << (int)actual[i] << " != " << (int)expected[i] << ")\n";
            testsFailed++;
            return;
        }
    }

    std::cout << "[PASS] " << testName << "\n";
    testsPassed++;
}

void testSingleFrame() {
    printHeader("Single Frame Parsing");
    try {
        const std::vector<std::string> frames = {
            "7400210C00000000000"
        };

        const auto results = parseCanFrames(frames);

        check(results.size() == 1, "Correct number of parsed messages");
        if (!results.empty()) {
            check(results[0].canId == 0x740, "Parsed correct CAN ID");
            checkPayload(results[0].payload, {0x10, 0xC0}, "Parsed correct payload bytes");
        }
    } catch (const std::exception& e) {
        check(false, "Single Frame Execution", std::string("Unexpected exception: ") + e.what());
    }
}

void testMultiFrameAssembly() {
    printHeader("Multi-Frame Assembly");
    try {
        const std::vector<std::string> frames = {
            "760101A618339484D31", // First frame
            "7602141341101000265", // Consecutive 1
            "7602295616529201203", // Consecutive 2
            "76023000000000080AA"  // Consecutive 3
        };

        const auto results = parseCanFrames(frames);

        check(results.size() == 1, "Assembled into exactly one message");
        if (!results.empty()) {
            check(results[0].canId == 0x760, "Correct CAN ID for multi-frame");
            check(results[0].payload.size() == 26, "Correct total payload size");
            check(results[0].payload[0] == 0x61, "First byte matches FF");
            check(results[0].payload[25] == 0x80, "Last byte matches CF3 before padding");
        }
    } catch (const std::exception& e) {
        check(false, "Multi-Frame Execution", e.what());
    }
}

void testInvalidLengthThrows() {
    printHeader("Error Handling: Invalid Length");
    bool caughtException = false;

    try {
        const std::vector<std::string> frames = {
            "7400210C0000"
        };
        parseCanFrames(frames);
    } catch (const std::runtime_error&) {
        caughtException = true;
    }

    check(caughtException, "Throws runtime_error on invalid string length");
}

void testOutOfOrderConsecutiveFrameThrows() {
    printHeader("Error Handling: Out of Order CF");
    bool caughtException = false;

    try {
        const std::vector<std::string> frames = {
            "760101A618339484D31", // FF starts session
            "7602295616529201203"  // CF2 sent instead of expected CF1
        };
        parseCanFrames(frames);
    } catch (const std::runtime_error&) {
        caughtException = true;
    }

    check(caughtException, "Throws runtime_error on wrong sequence number");
}

void testFlowControlAbort() {
    printHeader("Protocol Logic: Flow Control Abort");
    bool caughtException = false;

    try {
        const std::vector<std::string> frames = {
            "760101A618339484D31", // FF starts session
            "7403200000000000000", // FC with status 2 (Overflow) aborts session
            "7602141341101000265"  // This CF1 is now orphaned
        };
        parseCanFrames(frames);
    } catch (const std::runtime_error&) {
        caughtException = true;
    }

    check(caughtException, "Session successfully aborted by invalid Flow Control");
}

void testInterleavedMessages() {
    printHeader("Interleaved Messages");
    try {
        const std::vector<std::string> frames = {
            "7E810156181314E3441", // Message1 FF
            "760101A618339484D31", // Message2 FF
            "7E8214C33415039464E", // Message1 CF1
            "7602141341101000265", // Message2 CF1
            "7602295616529201203", // Message2 CF2
            "76023000000000080AA", // Message2 CF3 (Finishes)
            "7E82239303031363300", // Message1 CF2
            "7E82300AAAAAAAAAAAA"  // Message1 CF3 (Finishes)
        };

        const auto results = parseCanFrames(frames);

        check(results.size() == 2, "Assembled two distinct messages");
        if (results.size() == 2) {
            check(results[0].canId == 0x760, "First finished message is Message2 (0x760)");
            check(results[0].payload.size() == 26, "Message2 payload size is correct");
            check(results[1].canId == 0x7E8, "Second finished message is Message1 (0x7E8)");
            check(results[1].payload.size() == 21, "Message1 payload size is correct");
        }
    } catch (const std::exception& e) {
        check(false, "Interleaved Messages Execution", e.what());
    }
}

int main() {
    std::cout << "Starting CAN Parser Test Suite...\n";

    testSingleFrame();
    testMultiFrameAssembly();
    testInvalidLengthThrows();
    testOutOfOrderConsecutiveFrameThrows();
    testFlowControlAbort();
    testInterleavedMessages();

    std::cout << "\n========================================\n";
    std::cout << "Test Run Complete.\n";
    std::cout << "Passed: " << testsPassed << "\n";
    std::cout << "Failed: " << testsFailed << "\n";
    std::cout << "========================================\n";

    return (testsFailed == 0) ? 0 : 1;
}