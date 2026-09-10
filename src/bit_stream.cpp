#include "bit_stream.h"

// ─── BitWriter ────────────────────────────────────────────────────────────────

void BitWriter::write_bit(bool bit) {
    if (bit) {
        current_byte_ |= static_cast<std::uint8_t>(1 << (7 - bit_index_));
    }

    ++bit_index_;
    ++total_bits_;

    // Once 8 bits have accumulated, flush the full byte to the buffer.
    if (bit_index_ == 8) {
        data_.push_back(current_byte_);
        current_byte_ = 0;
        bit_index_    = 0;
    }
}

void BitWriter::write_bits(std::uint64_t value, std::uint8_t count) {
    // Write bits from most-significant bit (MSB) to least-significant bit (LSB).
    for (int i = static_cast<int>(count) - 1; i >= 0; --i) {
        bool bit = (value >> i) & 1ULL;
        write_bit(bit);
    }
}

void BitWriter::flush() {
    // Flush any pending incomplete byte (padded with trailing zeroes).
    if (bit_index_ > 0) {
        data_.push_back(current_byte_);
        current_byte_ = 0;
        bit_index_    = 0;
    }
}

const std::vector<std::uint8_t>& BitWriter::bytes() const {
    return data_;
}

std::size_t BitWriter::total_bits() const {
    return total_bits_;
}

// ─── BitReader ────────────────────────────────────────────────────────────────

BitReader::BitReader(std::span<const std::uint8_t> data)
    : data_(data) {}

std::optional<bool> BitReader::read_bit() {
    if (byte_pos_ >= data_.size()) {
        return std::nullopt; // End-of-stream reached
    }

    bool bit = (data_[byte_pos_] >> (7 - bit_index_)) & 1;
    ++bit_index_;

    if (bit_index_ == 8) {
        bit_index_ = 0;
        ++byte_pos_;
    }

    return bit;
}

std::optional<std::uint64_t> BitReader::read_bits(std::uint8_t count) {
    if (count == 0) {
        return 0;
    }

    std::uint64_t result = 0;
    for (std::uint8_t i = 0; i < count; ++i) {
        auto bit = read_bit();
        if (!bit) {
            return std::nullopt;
        }
        result = (result << 1) | (*bit ? 1ULL : 0ULL);
    }

    return result;
}

bool BitReader::empty() const {
    return byte_pos_ >= data_.size();
}