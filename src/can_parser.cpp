#include "can_parser.h"

#include <map>
#include <stdexcept>
#include <format>
#include <charconv>
#include <algorithm>

static constexpr std::size_t expectedFrameLength{19};
static constexpr std::size_t messageTypeIndex {2};

enum MessageType {
    SingleFrame = 0,
    FirstFrame = 1,
    ConsecutiveFrame = 2,
    FlowControlFrame = 3
};

class CanParser final {
public:
    void addFrame(std::string_view frame);

    const std::vector<CanMessage>& getResult() const {
        return parsedMessages;
    }

private:
    struct PendingMessage {
        uint16_t expectedSize;
        uint8_t expectedIndex {1};
        CanBuffer payload;
    };

    CanBuffer stringToBuffer(std::string_view frame) const;
    uint16_t getCanId(const CanBuffer& frame) const;
    void parseSingleFrame(const CanBuffer& frame);
    void parseFirstFrame(const CanBuffer& frame);
    void parseConsecutiveFrame(const CanBuffer& frame);
    void parseFlowControlFrame(const CanBuffer& frame);

    std::vector<CanMessage> parsedMessages;
    std::map<uint16_t, PendingMessage> pendingMessages;
};

CanBuffer CanParser::stringToBuffer(std::string_view frame) const {
    CanBuffer buffer;
    buffer.reserve((expectedFrameLength + 1) / 2);

    uint8_t byteValue = 0;
    auto [ptr, ec] = std::from_chars(frame.data(), frame.data() + 1, byteValue, 16);
    if (ec == std::errc()) {
        buffer.push_back(byteValue);
    } else {
        throw std::runtime_error(std::format("Invalid hexadecimal character in '{}'", frame));
    }

    for (size_t i = 1; i < frame.length(); i += 2) {
        auto [ptr, ec] = std::from_chars(frame.data() + i, frame.data() + i + 2, byteValue, 16);
        if (ec == std::errc()) {
            buffer.push_back(byteValue);
        } else {
            throw std::runtime_error(std::format("Invalid hexadecimal character in '{}'", frame));
        }
    }

    return buffer;
}

void CanParser::addFrame(std::string_view frame) {
    if (frame.size() != expectedFrameLength) {
        throw std::runtime_error(std::format("Incorrect frame length {} '{}'", frame.size(), frame));
    }

    CanBuffer buffer {stringToBuffer(frame)};

    if (buffer.size() != (expectedFrameLength + 1) / 2) {
        throw std::runtime_error(std::format("Incorrect frame length {} '{}'", buffer.size(), frame));
    }

    switch (buffer[messageTypeIndex] >> 4) {
    case MessageType::SingleFrame: {
        parseSingleFrame(buffer);
        break;
    }
    case MessageType::FirstFrame: {
        parseFirstFrame(buffer);
        break;
    }
    case MessageType::ConsecutiveFrame: {
        parseConsecutiveFrame(buffer);
        break;
    }
    case MessageType::FlowControlFrame: {
        parseFlowControlFrame(buffer);
        break;
    }
    default:
        break;
    }
}

uint16_t CanParser::getCanId(const CanBuffer& frame) const {
    return (frame[0] << 8) + frame[1];
}

void CanParser::parseSingleFrame(const CanBuffer& frame) {
    CanMessage result;
    result.canId = getCanId(frame);

    static constexpr auto payloadStart {3};
    static constexpr auto maxPayloadSize {7};
    const auto payloadSize {frame[messageTypeIndex] & 0xF};
    if (payloadSize > maxPayloadSize) {
        throw std::runtime_error(std::format("Received incorrect size for single message '{}'", payloadSize));
    }

    result.payload.insert(result.payload.end(), frame.begin() + payloadStart, frame.begin() + payloadStart + payloadSize);

    parsedMessages.emplace_back(std::move(result));
}

void CanParser::parseFirstFrame(const CanBuffer& frame) {
    const auto canId = getCanId(frame);

    if (pendingMessages.contains(canId)) {
        throw std::runtime_error(std::format("Received new first frame for an existing message '{:X}'", canId));
    }

    static constexpr auto payloadSizeIndex {3};
    PendingMessage message;
    message.expectedSize = ((frame[messageTypeIndex] & 0xF) << 8) + frame[payloadSizeIndex];
    message.payload.reserve(message.expectedSize);

    static constexpr auto payloadStart {4};
    static constexpr auto payloadSize {6};
    if (message.expectedSize <= payloadSize) {
        CanMessage result;
        result.canId = canId;
        result.payload.insert(result.payload.end(), frame.begin() + payloadStart, frame.begin() + payloadStart + message.expectedSize);
        parsedMessages.emplace_back(std::move(result));
        return;
    }

    message.payload.insert(message.payload.end(), frame.begin() + payloadStart, frame.begin() + payloadStart + payloadSize);
    pendingMessages.emplace(canId, std::move(message));
}

void CanParser::parseConsecutiveFrame(const CanBuffer& frame) {
    const auto canId {getCanId(frame)};

    if (!pendingMessages.contains(canId)) {
        throw std::runtime_error(std::format("Received consecutive frame for non existent message '{:X}'", canId));
    }

    auto& message = pendingMessages.at(canId);
    static constexpr auto indexLocation {2};
    const auto frameIndex {frame[indexLocation] & 0xF};
    if (frameIndex != message.expectedIndex) {
        throw std::runtime_error(std::format("Received unexpected consecutive frame index '{}', expected index '{}'", frameIndex, message.expectedIndex));
    }

    static constexpr auto payloadStart {3};
    const auto remainingData {message.expectedSize - message.payload.size()};
    const auto payloadSize {std::min(remainingData, 7ul)};
    message.payload.insert(message.payload.end(), frame.begin() + payloadStart, frame.begin() + payloadStart + payloadSize);
    message.expectedIndex = (message.expectedIndex + 1) & 0xF;

    if (message.payload.size() == message.expectedSize) {
        parsedMessages.emplace_back(canId, std::move(message.payload));
        pendingMessages.erase(canId);
        return;
    }
}

void CanParser::parseFlowControlFrame(const CanBuffer& frame) {
    const auto flag {frame[messageTypeIndex] & 0xF};
    static constexpr auto abort {2};
    if (flag == abort) {
        const auto canId {getCanId(frame)};
        pendingMessages.erase(canId + 0x20);
        pendingMessages.erase(canId + 0x8);
    }
}

std::vector<CanMessage> parseCanFrames(const std::vector<std::string>& frames) {
    CanParser parser;
    for (const auto& frame : frames) {
        parser.addFrame(frame);
    }

    return parser.getResult();
}