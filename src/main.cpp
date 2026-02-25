#include <iostream>
#include <fstream>
#include <iomanip>

#include "can_parser.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Missing file for input" << std::endl;
        return 1;
    }

    std::ifstream file(argv[1]);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open the file.\n";
        return 1;
    }

    std::vector<std::string> frames;
    std::string line;
    while (std::getline(file, line)) {
        frames.emplace_back(std::move(line));
    }

    try {
        const auto parsedFrames = parseCanFrames(frames);

        for (const auto& frame : parsedFrames) {
            std::cout << std::hex << frame.canId << ": ";
            for (const int byte : frame.payload) {
                std::cout << std::hex << std::setw(2) << std::setfill('0') << byte;
            }
            std::cout << std::endl;
        }
    }
    catch (const std::runtime_error& ex) {
        std::cerr << ex.what();
    }

    return 0;
}
