#pragma once

#include <vector>
#include <string>
#include <cstdint>

using CanBuffer = std::vector<uint8_t>;

struct CanMessage {
    uint16_t canId;
    CanBuffer payload;
};

std::vector<CanMessage> parseCanFrames(const std::vector<std::string>& frames);