#include "IwiWaveletDecoder.h"

#include "IwiWaveletCodebooks.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <vector>

namespace image::wavelet
{
    namespace
    {
        /**
         * \brief The 12-bit lookup table used by the decoder.
         *
         * The game stores these tables in their expanded form, that is, 4096
         * {symbol, length} entries for every codebook. This makes the actual
         * decoding very cheap since the low 12 bits of the accumulator can be
         * used directly as an index.
         *
         * We don't store that expanded form in IwiWaveletCodebooks.h since most
         * of it is the same information repeated over and over. Instead, we
         * keep the codes themselves and rebuild the table here the first time
         * it is needed.
         *
         * Note that the codes are read least-significant bit first. As a
         * result, a code matches the low bits of the table index, with any
         * remaining high bits being lookahead for the following code.
         */
        class Lookup
        {
        public:
            explicit Lookup(const Codebook& codebook)
                : m_escape_bits(codebook.m_escape_bits),
                  m_escape_bias(codebook.m_escape_bias)
            {
                // Start with a known value mostly so that a mistake in
                // one of the recovered codebooks doesn't leave us with
                // uninitialized table entries. A complete codebook
                // should overwrite every entry below.
                //
                m_symbol.fill(0);
                m_length.fill(0);

                // Expand every code into all the 12-bit values for
                // which it is a prefix. Once this is done ReadCode()
                // doesn't have to walk a tree or compare codes one at a
                // time.
                //
                for (auto i = 0u; i < codebook.m_entry_count; i++)
                {
                    const auto& entry = codebook.m_entries[i];
                    Fill(entry.m_code, entry.m_length, entry.m_symbol);
                }

                // Treat the escape code exactly like an ordinary code
                // here. ReadCode() will recognize the special symbol
                // and consume the raw value that follows it.
                //
                Fill(codebook.m_escape_code,
                     codebook.m_escape_length,
                     ESCAPE);
            }

            // This is outside the range of any coefficient produced by
            // the codebooks, so it is safe to use as an internal
            // marker.
            //
            static constexpr std::int16_t ESCAPE = -32768;

            std::array<std::int16_t, 4096> m_symbol;
            std::array<std::uint8_t, 4096> m_length;
            unsigned m_escape_bits;
            std::int16_t m_escape_bias;

        private:
            void Fill(const unsigned code,
                      const unsigned length,
                      const std::int16_t symbol)
            {
                // This is perhaps the slightly non-obvious part of
                // rebuilding the table. Suppose, for example, that we
                // have a three-bit code. Since the decoder indexes with
                // 12 bits, that code has to appear once for every
                // possible value of the other nine bits.
                //
                // Since the stream is read least-significant bit first,
                // the code occupies the low bits. So mask those bits
                // and assign every index that has the expected value
                // there.
                //
                const auto mask = (1u << length) - 1u;

                for (auto index = 0u; index < 4096u; index++)
                {
                    if ((index & mask) == code)
                    {
                        m_symbol[index] = symbol;
                        m_length[index] =
                            static_cast<std::uint8_t>(length);
                    }
                }
            }
        };

        const Lookup& LookupA()
        {
            // There is no reason to build the expanded tables if no
            // wavelet image is ever decoded. So keep them as
            // function-local statics and let the first actual decode
            // pay the construction cost.
            //
            static const Lookup lookup(CODEBOOK_A);
            return lookup;
        }

        const Lookup& LookupB()
        {
            static const Lookup lookup(CODEBOOK_B);
            return lookup;
        }

        const Lookup& LookupC()
        {
            static const Lookup lookup(CODEBOOK_C);
            return lookup;
        }

        [[nodiscard]] std::uint8_t Clamp(const int value)
        {
            // Keep all the lifting arithmetic signed and only narrow
            // when the result is written back to the image. Some
            // coefficients do take the intermediate value outside the
            // byte range.
            //
            return static_cast<std::uint8_t>(
                std::clamp(value, 0, 255));
        }
    } // namespace

    Decoder::Decoder(const uint8_t* data,
                     const size_t dataSize,
                     const unsigned channelCount,
                     const unsigned outputStride)
        : m_data(data),
          m_data_size(dataSize),
          m_byte_pos(0u),
          m_accumulator(0u),
          m_bit_offset(0u),
          m_started(false),
          m_overrun(false),
          m_channel_count(channelCount),
          m_output_stride(outputStride),
          m_channel_offset{0u, 1u, 2u, 3u}
    {
        // There are paths below that refer to the last encoded channel
        // and to the first three colour channels, so keep the format
        // limits here.
        //
        assert(channelCount >= 1u && channelCount <= 4u);

        // The output may have padding or an extra alpha component, but
        // it must at least have room for all the channels carried by
        // the stream.
        //
        assert(outputStride >= channelCount);
    }

    size_t Decoder::BytesConsumed() const
    {
        // Note that this is the bit reader's byte cursor. Near the end
        // of a valid stream it can include the zero-filled lookahead
        // described in Window(), which is also what the game appears to
        // report.
        //
        return m_byte_pos;
    }

    bool Decoder::EnsureAvailable(const size_t bytes) const
    {
        // Only use this for bytes that must physically exist in the
        // input. Bitstream refill is allowed to look past the end and
        // is handled by Window() below.
        //
        return m_byte_pos + bytes <= m_data_size;
    }

    std::uint32_t Decoder::Window() const
    {
        // The bit reader keeps a 16-bit accumulator and refills its
        // high end from a 24-bit window starting at m_byte_pos. The
        // game appears to form this window unconditionally, including
        // for the last few codes in the stream.
        //
        // Those final codes are already complete in the accumulator.
        // The bytes past the buffer are only used to refill bits that
        // will never be observed. Returning zero there gives us the
        // same useful result without relying on an out-of-bounds read.
        //
        const auto at = [this](const size_t offset) -> std::uint32_t
        {
            const auto index = m_byte_pos + offset;
            return index < m_data_size ? m_data[index] : 0u;
        };

        // The stream is little-endian and is consumed from the low end,
        // so assemble the refill window in the same order.
        //
        return at(0u) |
               (at(1u) << 8u) |
               (at(2u) << 16u);
    }

    void Decoder::Start()
    {
        // DecodeLevel() calls us for every transformed level, while the
        // bitstream itself is continuous across those levels. So only
        // load the initial accumulator once and leave it alone on later
        // calls.
        //
        if (m_started)
            return;

        // Unlike the lookahead in Window(), these first two bytes are
        // the initial accumulator and must be present in the file.
        //
        if (!EnsureAvailable(2u))
        {
            m_overrun = true;
            return;
        }

        // Codes are consumed from the low bits, so load the initial
        // word in little-endian order.
        //
        m_accumulator = static_cast<std::uint16_t>(
            m_data[m_byte_pos] |
            (m_data[m_byte_pos + 1] << 8));

        // m_byte_pos points at the refill window, not at the first byte
        // still represented by the accumulator. We have just moved two
        // bytes into the accumulator, so the refill window starts
        // immediately after them.
        //
        m_byte_pos += 2u;
        m_bit_offset = 0u;
        m_started = true;
    }

    unsigned Decoder::ReadBit()
    {
        // This bit reservoir is easy to get subtly wrong, so let's
        // spell out the state we maintain. The next unread bit is
        // always bit zero of m_accumulator. m_byte_pos and m_bit_offset
        // identify the next bit that should enter at bit 15 after the
        // accumulator is shifted.
        //
        const auto result =
            static_cast<unsigned>(m_accumulator & 1u);

        const auto window = Window();

        // Drop the bit we are returning and pull one replacement bit
        // into the top. Shifting the window by m_bit_offset places its
        // next unread bit at bit zero, after which the shift by 15 puts
        // it at the top of the accumulator.
        //
        // The cast to uint16_t is intentional since only that one low
        // bit of the shifted window belongs in the accumulator.
        //
        m_accumulator = static_cast<std::uint16_t>(
            (m_accumulator >> 1) |
            ((window >> m_bit_offset) << 15));

        // Advance the refill position. Once the offset reaches eight,
        // move to the next byte and continue at bit zero of that byte.
        //
        const auto total = m_bit_offset + 1u;

        m_byte_pos += total >> 3u;
        m_bit_offset = total & 7u;

        // Do not treat the zero-filled lookahead as an overrun. A real
        // overrun is only possible when DecodeRawLevel() needs a byte
        // that isn't present.
        //
        return result;
    }

    int Decoder::ReadCode(const Codebook& codebook)
    {
        // Passing the compact codebook and then selecting the expanded
        // lookup by address may look a little roundabout.
        //
        const Lookup* lookup;

        if (&codebook == &CODEBOOK_A)
            lookup = &LookupA();
        else if (&codebook == &CODEBOOK_B)
            lookup = &LookupB();
        else
            lookup = &LookupC();

        // The next code starts at bit zero. Use the next 12 bits as the
        // table index and let the selected entry tell us how many of
        // them actually belong to this code.
        //
        const auto index =
            static_cast<unsigned>(m_accumulator) & 0xFFFu;

        const auto length = lookup->m_length[index];
        const auto symbol = lookup->m_symbol[index];

        // ReadBit() could be called repeatedly here, but the game
        // consumes a complete code in one operation and it is worth
        // preserving that shape. Besides being cheaper, it makes the
        // relationship between the lookup length and the reservoir
        // movement explicit.
        //
        const auto consume = [this](const unsigned bits)
        {
            const auto window = Window();

            // Remove the consumed bits from the low end and refill the
            // same number at the high end. As in ReadBit(), narrowing
            // to uint16_t discards everything outside the reservoir.
            //
            m_accumulator = static_cast<std::uint16_t>(
                (m_accumulator >> bits) |
                ((window >> m_bit_offset) << (16u - bits)));

            // m_bit_offset is always smaller than eight before this
            // operation. Adding the code length tells us both how many
            // complete bytes were crossed and where the next refill
            // begins in the current byte.
            //
            const auto total = m_bit_offset + bits;

            m_byte_pos += total >> 3u;
            m_bit_offset = total & 7u;
        };

        consume(length);

        if (symbol != Lookup::ESCAPE)
            return symbol;

        // The escape code means the coefficient was outside the range
        // assigned a variable-length symbol. What follows is a
        // fixed-width unsigned value with a codebook-specific bias.
        //
        // Note that consume(length) has already removed the escape
        // code, so the payload is now sitting at the low end of the
        // accumulator.
        //
        const auto rawBits = lookup->m_escape_bits;
        const auto raw =
            static_cast<unsigned>(m_accumulator) &
            ((1u << rawBits) - 1u);

        consume(rawBits);

        return static_cast<int>(raw) - lookup->m_escape_bias;
    }

    void Decoder::DecodeRawLevel(uint8_t* dest,
                                 const unsigned width,
                                 const unsigned height)
    {
        // The smallest level is where the wavelet pyramid starts. There
        // is no smaller parent from which it could be reconstructed, so
        // the stream stores its samples literally before the coded
        // levels.
        //
        // A dimension that has collapsed to zero still denotes the one
        // sample at the tip of the pyramid.
        //
        const auto pixels =
            std::max(width, 1u) * std::max(height, 1u);

        for (auto i = 0u; i < pixels; i++)
        {
            // Samples are interleaved per pixel in the stream. Use the
            // channel offsets here as well so this path writes the same
            // destination layout as the transformed path.
            //
            for (auto channel = 0u;
                 channel < m_channel_count;
                 channel++)
            {
                // These are real sample bytes, so zero-filled lookahead
                // is not applicable. Bail out as soon as the source is
                // truncated.
                //
                if (!EnsureAvailable(1u))
                {
                    m_overrun = true;
                    return;
                }

                dest[m_channel_offset[channel]] =
                    m_data[m_byte_pos++];
            }

            // The decoder is normally asked to expand RGB into an RGBA
            // output. In that case the extra component doesn't exist in
            // the stream, so make it opaque here just like we do for
            // reconstructed RGB blocks.
            //
            if (m_output_stride != m_channel_count)
                dest[m_channel_offset[3]] = 0xFFu;

            dest += m_output_stride;
        }
    }

    bool Decoder::DecodeLevel(const uint8_t* parent,
                              uint8_t* dest,
                              const unsigned width,
                              const unsigned height)
    {
        // First deal with the tip of the pyramid. Once either dimension
        // has reached one there is no 2x2 level to reconstruct and the
        // stream switches to the literal representation described
        // above.
        //
        if (width <= 1u || height <= 1u)
        {
            DecodeRawLevel(dest, width, height);
            return !m_overrun;
        }

        // Start the bit reader on the first transformed level. Start()
        // is a no-op after that since all the levels share one
        // continuous stream.
        //
        Start();

        const auto stride = m_output_stride;
        const auto rowStride = stride * width;

        // The first bit of each level is a little surprising. It tells
        // us whether the already-decoded parent has another correction
        // pass associated with this child level.
        //
        // So while parent is normally only a predictor, in this case
        // the format asks us to update it before using it.
        //
        if (ReadBit())
        {
            auto* refine = const_cast<uint8_t*>(parent);

            // Every parent sample expands into a 2x2 child block. There
            // are therefore width * height / 4 parent samples to
            // refine.
            //
            const auto blocks = width * height / 4u;

            for (auto i = 0u; i < blocks; i++)
            {
                for (auto channel = 0u;
                     channel < m_channel_count;
                     channel++)
                {
                    const auto offset = m_channel_offset[channel];

                    // Refinement coefficients use table A for every
                    // channel. Apply the correction in int and clamp on
                    // the way back to the byte-sized parent sample.
                    //
                    refine[offset] = Clamp(
                        refine[offset] + ReadCode(CODEBOOK_A));
                }

                refine += stride;
            }
        }

        // Each parent pixel produces one 2x2 block.
        //
        const auto blocksX = ((width - 1u) >> 1u) + 1u;
        const auto blocksY = ((height - 1u) >> 1u) + 1u;

        const auto* parentRow = parent;
        auto* destRow = dest;

        for (auto by = 0u; by < blocksY; by++)
        {
            const auto* parentPixel = parentRow;
            auto* destPixel = destRow;

            for (auto bx = 0u; bx < blocksX; bx++)
            {
                // This is the actual inverse lifting step. For one
                // channel we have the parent sample, three detail
                // coefficients, and one parity bit. Together they
                // recover the four samples in the corresponding 2x2
                // destination block.
                //
                const auto lift =
                    [&](const unsigned offset,
                        const unsigned parity,
                        const int c1,
                        const int c2,
                        const int c3)
                {
                    // The equations work with twice the parent value so
                    // that the additions stay in integer space until
                    // the final shift.
                    //
                    const auto p =
                        2 * static_cast<int>(parentPixel[offset]);

                    auto* out = destPixel + offset;

                    // The parity bit belongs to this first result.
                    // Without it, the right shift would lose the
                    // odd/even choice made by the encoder and the
                    // transform would not be reversible.
                    //
                    out[0] = Clamp(
                        static_cast<int>(parity) +
                        ((p + c1 + c2 + c3) >> 1));

                    out[stride] = Clamp(
                        (p + c1 - c3 - c2) >> 1);

                    out[rowStride] = Clamp(
                        (p + c2 - c3 - c1) >> 1);

                    out[rowStride + stride] = Clamp(
                        (p + c3 - c2 - c1) >> 1);
                };

                // Now decode the colour part of the block. Channel zero
                // carries the base set of colour coefficients and uses
                // table B.
                //
                // A one-channel image has no shared colour base, so it
                // skips this section and is decoded by the
                // independent-channel path below.
                //
                if (m_channel_count != 1u)
                {
                    const auto parity = ReadBit();
                    const auto c1 = ReadCode(CODEBOOK_B);
                    const auto c2 = ReadCode(CODEBOOK_B);
                    const auto c3 = ReadCode(CODEBOOK_B);

                    lift(m_channel_offset[0],
                         parity,
                         c1,
                         c2,
                         c3);

                    // Channels one and two are correlated with channel
                    // zero, so the stream stores their detail
                    // coefficients as deltas. Table C has the wider
                    // range needed for those deltas.
                    //
                    // Add the base coefficient back here and pass the
                    // complete values to lift(), which then doesn't
                    // need to know anything about the inter-channel
                    // coding.
                    //
                    if (m_channel_count >= 3u)
                    {
                        for (auto channel = 1u;
                             channel <= 2u;
                             channel++)
                        {
                            const auto p2 = ReadBit();
                            const auto d1 =
                                ReadCode(CODEBOOK_C) + c1;
                            const auto d2 =
                                ReadCode(CODEBOOK_C) + c2;
                            const auto d3 =
                                ReadCode(CODEBOOK_C) + c3;

                            lift(m_channel_offset[channel],
                                 p2,
                                 d1,
                                 d2,
                                 d3);
                        }
                    }
                }

                if (m_channel_count == 3u)
                {
                    // There is no alpha channel in an RGB stream. If
                    // the caller asked for a four-byte output, fill
                    // alpha for all four pixels produced by this block.
                    //
                    if (m_output_stride != 3u)
                    {
                        auto* out =
                            destPixel + m_channel_offset[3];

                        out[0] = 0xFFu;
                        out[stride] = 0xFFu;
                        out[rowStride] = 0xFFu;
                        out[rowStride + stride] = 0xFFu;
                    }
                }
                else
                {
                    // The last encoded channel is kept separate from
                    // the colour delta scheme and uses table A. For
                    // RGBA this is alpha. For a one-channel image it is
                    // the image itself.
                    //
                    // Two-channel images follow the same rule, with the
                    // second channel occupying this independent slot.
                    //
                    const auto offset =
                        m_channel_offset[m_channel_count - 1u];

                    const auto parity = ReadBit();
                    const auto c1 = ReadCode(CODEBOOK_A);
                    const auto c2 = ReadCode(CODEBOOK_A);
                    const auto c3 = ReadCode(CODEBOOK_A);

                    lift(offset,
                         parity,
                         c1,
                         c2,
                         c3);
                }

                // We have produced two destination pixels horizontally
                // and consumed one predictor from the parent row.
                //
                destPixel += 2u * stride;
                parentPixel += stride;
            }

            // And similarly, one parent row produces two destination
            // rows. Since the parent is half as wide, half the
            // destination row stride advances parentRow by one complete
            // row.
            //
            destRow += 2u * rowStride;
            parentRow += rowStride / 2u;
        }

        // The bit reader's zero-filled lookahead is part of normal
        // decoding. m_overrun is reserved for a physical byte that the
        // raw level needed and could not read.
        //
        return !m_overrun;
    }
} // namespace image::wavele
