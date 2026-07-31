#pragma once

#include <cstddef>
#include <cstdint>

namespace image::wavelet
{
    struct Codebook;

    /**
     * \brief Decodes the IWI wavelet formats, IMG_FORMAT_WAVELET_RGBA through _ALPHA.
     *
     * \note Format 10 (_ALPHA) has no sample anywhere in retail, but it runs the same
     * single channel path as format 9 and differs only in which ImageFormat the result is reported
     * as, so it is exercised indirectly.
     */
    class Decoder
    {
    public:
        Decoder(const uint8_t* data, size_t dataSize, unsigned channelCount, unsigned outputStride);

        /**
         * \brief Decodes one mip level, given the level below it.
         *
         * \param parent The already decoded next-smaller level, or nullptr for the smallest, which
         *        carries raw samples rather than coefficients.
         * \param dest Receives width * height * outputStride bytes.
         * \return false if the bitstream ran out, which means the file is truncated or the
         *         dimensions passed in do not match what it was encoded with.
         */
        bool DecodeLevel(const uint8_t* parent, uint8_t* dest, unsigned width, unsigned height);

        /** \brief How many bytes of the input have been consumed so far. */
        [[nodiscard]] size_t BytesConsumed() const;

    private:
        [[nodiscard]] bool EnsureAvailable(size_t bytes) const;
        [[nodiscard]] std::uint32_t Window() const;
        void Start();
        unsigned ReadBit();
        int ReadCode(const Codebook& codebook);
        void DecodeRawLevel(uint8_t* dest, unsigned width, unsigned height);

        const uint8_t* m_data;
        size_t m_data_size;
        size_t m_byte_pos;
        uint16_t m_accumulator;
        unsigned m_bit_offset;
        bool m_started;
        bool m_overrun;

        unsigned m_channel_count;
        unsigned m_output_stride;
        unsigned m_channel_offset[4];
    };
} // namespace image::wavelet
