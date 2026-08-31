#include "bike/stream_parser.hpp"
#include "bike_protocol/codec.hpp"

namespace bike {

bool StreamParser::push(std::uint8_t byte, Packet& out_packet) {
    if (length_ == 0) {
        if (byte != kSyncByte1) return false;
        buffer_[length_++] = byte;
        return false;
    }

    if (length_ == 1) {
        if (byte != kSyncByte2) {
            length_ = (byte == kSyncByte1) ? 1 : 0;
            if (length_ == 1) buffer_[0] = byte;
            return false;
        }
        buffer_[length_++] = byte;
        return false;
    }

    if (length_ >= buffer_.size()) {
        reset();
        return false;
    }

    buffer_[length_++] = byte;

    if (length_ == kHeaderSize) {
        const std::uint16_t payload_length = buffer_[9];
        if (payload_length > kMaxPayloadSize) {
            reset();
            return false;
        }
        expected_ = kHeaderSize + payload_length + kCrcSize;
    }

    if (expected_ != 0 && length_ == expected_) {
        const auto status = decode_packet(buffer_.data(), length_, out_packet);
        reset();
        return status == DecodeStatus::Ok;
    }

    return false;
}

void StreamParser::reset() {
    length_ = 0;
    expected_ = 0;
}

} // namespace bike
