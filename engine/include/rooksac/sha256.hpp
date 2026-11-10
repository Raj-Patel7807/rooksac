#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace rooksac {
namespace crypto {

/// \brief Computes standard cryptographic SHA-256 digests in pure portable C++20.
class Sha256 {
public:
    static inline constexpr std::size_t kDigestSize = 32;

    Sha256() noexcept { reset(); }

    void reset() noexcept {
        state_[0] = 0x6a09e667U;
        state_[1] = 0xbb67ae85U;
        state_[2] = 0x3c6ef372U;
        state_[3] = 0xa54ff53aU;
        state_[4] = 0x510e527fU;
        state_[5] = 0x9b05688cU;
        state_[6] = 0x1f83d9abU;
        state_[7] = 0x5be0cd19U;
        count_ = 0;
        bufferLength_ = 0;
    }

    void update(const void* data, std::size_t length) noexcept {
        const auto* input = static_cast<const std::uint8_t*>(data);
        count_ += length;

        std::size_t i = 0;
        if (bufferLength_ > 0) {
            while (bufferLength_ < 64 && i < length) {
                buffer_[bufferLength_++] = input[i++];
            }
            if (bufferLength_ == 64) {
                transform(buffer_.data());
                bufferLength_ = 0;
            }
        }

        while (i + 64 <= length) {
            transform(input + i);
            i += 64;
        }

        while (i < length) {
            buffer_[bufferLength_++] = input[i++];
        }
    }

    [[nodiscard]] std::array<std::uint8_t, kDigestSize> finalize() noexcept {
        std::uint64_t totalBits = count_ * 8ULL;

        // Append 0x80 byte
        buffer_[bufferLength_++] = 0x80U;
        if (bufferLength_ > 56) {
            while (bufferLength_ < 64) {
                buffer_[bufferLength_++] = 0;
            }
            transform(buffer_.data());
            bufferLength_ = 0;
        }

        while (bufferLength_ < 56) {
            buffer_[bufferLength_++] = 0;
        }

        // Append bit count in big-endian
        for (int i = 7; i >= 0; --i) {
            buffer_[bufferLength_++] = static_cast<std::uint8_t>((totalBits >> (i * 8)) & 0xFFU);
        }

        transform(buffer_.data());

        std::array<std::uint8_t, kDigestSize> digest{};
        for (std::size_t i = 0; i < 8; ++i) {
            digest[i * 4 + 0] = static_cast<std::uint8_t>((state_[i] >> 24) & 0xFFU);
            digest[i * 4 + 1] = static_cast<std::uint8_t>((state_[i] >> 16) & 0xFFU);
            digest[i * 4 + 2] = static_cast<std::uint8_t>((state_[i] >> 8) & 0xFFU);
            digest[i * 4 + 3] = static_cast<std::uint8_t>(state_[i] & 0xFFU);
        }

        return digest;
    }

private:
    static inline constexpr std::uint32_t K[64] = {
        0x428a2f98U, 0x71374491U, 0xb5c0fbcfU, 0xe9b5dba5U, 0x3956c25bU, 0x59f111f1U, 0x923f82a4U, 0xab1c5ed5U,
        0xd807aa98U, 0x12835b01U, 0x243185beU, 0x550c7dc3U, 0x72be5d74U, 0x80deb1feU, 0x9bdc06a7U, 0xc19bf174U,
        0xe49b69c1U, 0xefbe4786U, 0x0fc19dc6U, 0x240ca1ccU, 0x2de92c6fU, 0x4a7484aaU, 0x5cb0a9dcU, 0x76f988daU,
        0x983e5152U, 0xa831c66dU, 0xb00327c8U, 0xbf597fc7U, 0xc6e00bf3U, 0xd5a79147U, 0x06ca6351U, 0x14292967U,
        0x27b70a85U, 0x2e1b2138U, 0x4d2c6dfcU, 0x53380d13U, 0x650a7354U, 0x766a0abbU, 0x81c2c92eU, 0x92722c85U,
        0xa2bfe8a1U, 0xa81a664bU, 0xc24b8b70U, 0xc76c51a3U, 0xd192e819U, 0xd6990624U, 0xf40e3585U, 0x106aa070U,
        0x19a4c116U, 0x1e376c08U, 0x2748774cU, 0x34b0bcb5U, 0x391c0cb3U, 0x4ed8aa4aU, 0x5b9cca4fU, 0x682e6ff3U,
        0x748f82eeU, 0x78a5636fU, 0x84c87814U, 0x8cc70208U, 0x90befffaU, 0xa4506cebU, 0xbef9a3f7U, 0xc67178f2U
    };

    static constexpr std::uint32_t rotr(std::uint32_t x, std::uint32_t n) noexcept {
        return (x >> n) | (x << (32 - n));
    }
    static constexpr std::uint32_t ch(std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept {
        return (x & y) ^ (~x & z);
    }
    static constexpr std::uint32_t maj(std::uint32_t x, std::uint32_t y, std::uint32_t z) noexcept {
        return (x & y) ^ (x & z) ^ (y & z);
    }
    static constexpr std::uint32_t sig0(std::uint32_t x) noexcept {
        return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
    }
    static constexpr std::uint32_t sig1(std::uint32_t x) noexcept {
        return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
    }
    static constexpr std::uint32_t theta0(std::uint32_t x) noexcept {
        return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
    }
    static constexpr std::uint32_t theta1(std::uint32_t x) noexcept {
        return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
    }

    void transform(const std::uint8_t* block) noexcept {
        std::uint32_t w[64];
        for (std::size_t i = 0; i < 16; ++i) {
            w[i] = (static_cast<std::uint32_t>(block[i * 4 + 0]) << 24) |
                   (static_cast<std::uint32_t>(block[i * 4 + 1]) << 16) |
                   (static_cast<std::uint32_t>(block[i * 4 + 2]) << 8) |
                   static_cast<std::uint32_t>(block[i * 4 + 3]);
        }
        for (std::size_t i = 16; i < 64; ++i) {
            w[i] = theta1(w[i - 2]) + w[i - 7] + theta0(w[i - 15]) + w[i - 16];
        }

        std::uint32_t a = state_[0];
        std::uint32_t b = state_[1];
        std::uint32_t c = state_[2];
        std::uint32_t d = state_[3];
        std::uint32_t e = state_[4];
        std::uint32_t f = state_[5];
        std::uint32_t g = state_[6];
        std::uint32_t h = state_[7];

        for (std::size_t i = 0; i < 64; ++i) {
            std::uint32_t t1 = h + sig1(e) + ch(e, f, g) + K[i] + w[i];
            std::uint32_t t2 = sig0(a) + maj(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }

        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
        state_[5] += f;
        state_[6] += g;
        state_[7] += h;
    }

    std::uint32_t state_[8]{};
    std::uint64_t count_ = 0;
    std::array<std::uint8_t, 64> buffer_{};
    std::size_t bufferLength_ = 0;
};

}  // namespace crypto
}  // namespace rooksac

