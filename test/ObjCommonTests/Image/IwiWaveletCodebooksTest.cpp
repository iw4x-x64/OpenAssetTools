#include "Image/IwiWaveletCodebooks.h"

#include <catch2/catch_test_macros.hpp>
#include <set>
#include <vector>

using namespace image::wavelet;

namespace test::image::iwi_wavelet_codebooks
{
    std::vector<int> ExpandToLookup(const Codebook& codebook)
    {
        std::vector<int> claimedBy(4096, -1);

        const auto claim = [&claimedBy](const unsigned code, const unsigned length, const int owner)
        {
            const auto mask = (1u << length) - 1u;
            for (auto index = 0u; index < 4096u; index++)
            {
                if ((index & mask) == code)
                {
                    // A second claim on the same index means the code is not prefix free.
                    REQUIRE(claimedBy[index] == -1);
                    claimedBy[index] = owner;
                }
            }
        };

        for (auto i = 0u; i < codebook.m_entry_count; i++)
            claim(codebook.m_entries[i].m_code, codebook.m_entries[i].m_length, static_cast<int>(i));

        claim(codebook.m_escape_code, codebook.m_escape_length, static_cast<int>(codebook.m_entry_count));

        return claimedBy;
    }

    void CheckCodebook(const Codebook& codebook, const unsigned expectedSymbols)
    {
        REQUIRE(codebook.m_entry_count == expectedSymbols);

        // Prefix free, and every 12 bit index resolves. Together these mean the decoder can never
        // read a code it has no entry for, whatever the input bits are.
        const auto claimedBy = ExpandToLookup(codebook);
        for (const auto owner : claimedBy)
            REQUIRE(owner != -1);

        // Complete: the Kraft sum is exactly 1. Anything less would leave codes unassigned, anything
        // more would mean two symbols share a prefix.
        auto kraft = 0.0;
        for (auto i = 0u; i < codebook.m_entry_count; i++)
            kraft += 1.0 / static_cast<double>(1u << codebook.m_entries[i].m_length);
        kraft += 1.0 / static_cast<double>(1u << codebook.m_escape_length);
        REQUIRE(kraft == 1.0);

        // Every symbol appears once.
        std::set<int> symbols;
        for (auto i = 0u; i < codebook.m_entry_count; i++)
            REQUIRE(symbols.insert(codebook.m_entries[i].m_symbol).second);

        // A code must fit in the length it claims.
        for (auto i = 0u; i < codebook.m_entry_count; i++)
            REQUIRE(codebook.m_entries[i].m_code < (1u << codebook.m_entries[i].m_length));
    }

    TEST_CASE("IwiWaveletCodebooks: table A is a complete prefix free code", "[image][iwi]")
    {
        CheckCodebook(CODEBOOK_A, 107u);
        REQUIRE(CODEBOOK_A.m_escape_bits == 9u);
        REQUIRE(CODEBOOK_A.m_escape_bias == 255);
    }

    TEST_CASE("IwiWaveletCodebooks: table B is a complete prefix free code", "[image][iwi]")
    {
        CheckCodebook(CODEBOOK_B, 179u);
        REQUIRE(CODEBOOK_B.m_escape_bits == 9u);
        REQUIRE(CODEBOOK_B.m_escape_bias == 255);
    }

    TEST_CASE("IwiWaveletCodebooks: table C is a complete prefix free code", "[image][iwi]")
    {
        CheckCodebook(CODEBOOK_C, 79u);
        REQUIRE(CODEBOOK_C.m_escape_bits == 10u);
        REQUIRE(CODEBOOK_C.m_escape_bias == 510);
    }
} // namespace test::image::iwi_wavelet_codebooks
