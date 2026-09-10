#pragma once

#include <cstdint>
#include <vector>
#include <span>
#include <optional>

// Low-level bit-level stream for time-series compression (Gorilla).
// Enables writing and reading arbitrary bit sequences (1 to 64 bits) into byte buffers.

class BitWriter {
public:
    BitWriter() = default;

    // Writes a single bit (0 or 1).
    void write_bit(bool bit);

    // Writes the lowest `count` bits from `value` (count must be 1..64).
    void write_bits(std::uint64_t value, std::uint8_t count);

    // Flushes any remaining bits in the partial byte (padded with zeroes).
    void flush();

    // Returns a const reference to the packed byte buffer.
    const std::vector<std::uint8_t>& bytes() const;

    // Total count of bits written.
    std::size_t total_bits() const;

private:
    std::vector<std::uint8_t> data_;
    std::uint8_t              current_byte_ = 0;
    std::uint8_t              bit_index_    = 0; // Number of bits filled in current_byte_ (0..7)
    std::size_t               total_bits_   = 0;
};

class BitReader {
public:
    explicit BitReader(std::span<const std::uint8_t> data);

    // Reads a single bit. Returns nullopt if no more bits are available.
    std::optional<bool> read_bit();

    // Reads `count` bits (1..64). Returns nullopt on EOF.
    std::optional<std::uint64_t> read_bits(std::uint8_t count);

    // Returns true if all bits in the stream have been consumed.
    bool empty() const;

private:
    std::span<const std::uint8_t> data_;
    std::size_t                   byte_pos_  = 0;
    std::uint8_t                  bit_index_ = 0; // Current bit offset within data_[byte_pos_] (0..7)
};