#include "Zone/Stream/ZoneInputStream.h"

#include "Loading/Exception/InvalidOffsetBlockException.h"
#include "Loading/Exception/InvalidOffsetBlockOffsetException.h"
#include "Loading/ILoadingStream.h"

#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <memory>
#include <vector>

namespace
{
    class BufferLoadingStream final : public ILoadingStream
    {
    public:
        explicit BufferLoadingStream(std::vector<uint8_t> data)
            : m_data(std::move(data)),
              m_pos(0u)
        {
        }

        size_t Load(void* buffer, const size_t length) override
        {
            const auto available = m_data.size() - m_pos;
            const auto toRead = length < available ? length : available;

            std::memcpy(buffer, m_data.data() + m_pos, toRead);
            m_pos += toRead;

            return toRead;
        }

        int64_t Pos() override
        {
            return static_cast<int64_t>(m_pos);
        }

    private:
        std::vector<uint8_t> m_data;
        size_t m_pos;
    };

    class StreamFixture
    {
    public:
        StreamFixture(const unsigned pointerBitCount,
                      const unsigned blockBitCount,
                      const unsigned offsetBitCount,
                      std::vector<uint8_t> data,
                      const size_t blockSize = 0x1000u)
            : m_loading_stream(std::move(data))
        {
            for (auto i = 0u; i < BLOCK_COUNT; i++)
            {
                m_owned_blocks.emplace_back(std::make_unique<XBlock>("block", i, XBlockType::BLOCK_TYPE_NORMAL));
                m_owned_blocks.back()->Alloc(blockSize);
                m_blocks.emplace_back(m_owned_blocks.back().get());
            }

            m_stream = ZoneInputStream::Create(
                pointerBitCount, blockBitCount, offsetBitCount, m_blocks, 1u, m_loading_stream, m_memory, std::nullopt);
        }

        [[nodiscard]] ZoneInputStream& Stream() const
        {
            return *m_stream;
        }

        [[nodiscard]] const XBlock& Block(const size_t index) const
        {
            return *m_blocks[index];
        }

        static constexpr unsigned BLOCK_COUNT = 4u;

    private:
        BufferLoadingStream m_loading_stream;
        MemoryManager m_memory;
        std::vector<std::unique_ptr<XBlock>> m_owned_blocks;
        std::vector<XBlock*> m_blocks;
        std::unique_ptr<ZoneInputStream> m_stream;
    };

    [[nodiscard]] uintptr_t SerializedPointer(const unsigned block, const size_t offset, const unsigned offsetBitCount)
    {
        return ((static_cast<uintptr_t>(block) << offsetBitCount) | offset) + 1u;
    }

    TEST_CASE("ZoneInputStream: the block index sits at offsetBitCount, not at pointerBitCount - blockBitCount",
              "[zoneloading]")
    {
        struct Layout
        {
            const char* m_name;
            unsigned m_pointer_bits;
            unsigned m_block_bits;
            unsigned m_offset_bits;
        };

        const Layout layouts[]{
            {"IW3/IW4/IW5", 32u, 4u, 28u},
            {"T4/T5/T6", 32u, 3u, 29u},
            {"IW4MS", 64u, 4u, 28u},
        };

        for (const auto& layout : layouts)
        {
            if (layout.m_pointer_bits > sizeof(uintptr_t) * 8u)
                continue;

            INFO(layout.m_name);

            StreamFixture fixture(layout.m_pointer_bits, layout.m_block_bits, layout.m_offset_bits, {});
            auto& stream = fixture.Stream();

            REQUIRE(stream.GetPointerBitCount() == layout.m_pointer_bits);

            // Block 2, offset 0x40 must resolve to that exact spot in that exact block, whatever the
            // widths are. Resolving to the wrong block is the failure mode a derived shift produces.
            const auto serialized = SerializedPointer(2u, 0x40u, layout.m_offset_bits);
            const auto* resolved = stream.ConvertOffsetToPointerNative(reinterpret_cast<const void*>(serialized));

            REQUIRE(resolved == fixture.Block(2u).m_buffer.get() + 0x40u);
        }
    }

#ifdef ARCH_x64
    TEST_CASE("ZoneInputStream: a 64 bit pointer only uses its low 32 bits", "[zoneloading]")
    {
        StreamFixture fixture(64u, 4u, 28u, {});
        auto& stream = fixture.Stream();

        const auto withHighWordSet = SerializedPointer(2u, 0x40u, 28u) | (static_cast<uintptr_t>(1u) << 40u);

        REQUIRE_THROWS_AS(stream.ConvertOffsetToPointerNative(reinterpret_cast<const void*>(withHighWordSet)),
                          InvalidOffsetBlockException);
    }
#endif

    TEST_CASE("ZoneInputStream: an out of range block index is rejected", "[zoneloading]")
    {
        StreamFixture fixture(32u, 4u, 28u, {});
        auto& stream = fixture.Stream();

        const auto serialized = SerializedPointer(9u, 0x10u, 28u);

        REQUIRE_THROWS_AS(stream.ConvertOffsetToPointerNative(reinterpret_cast<const void*>(serialized)),
                          InvalidOffsetBlockException);
    }

    TEST_CASE("ZoneInputStream: an offset past the end of its block is rejected", "[zoneloading]")
    {
        StreamFixture fixture(32u, 4u, 28u, {}, 0x100u);
        auto& stream = fixture.Stream();

        const auto serialized = SerializedPointer(1u, 0x500u, 28u);

        REQUIRE_THROWS_AS(stream.ConvertOffsetToPointerNative(reinterpret_cast<const void*>(serialized)),
                          InvalidOffsetBlockOffsetException);
    }

    TEST_CASE("ZoneInputStream: alignment moves the cursor to the next multiple", "[zoneloading]")
    {
        StreamFixture fixture(32u, 4u, 28u, std::vector<uint8_t>(64u, 0xAAu));
        auto& stream = fixture.Stream();

        stream.PushBlock(0u);

        auto* first = stream.Alloc(1u);
        stream.LoadDataInBlock(first, 1u);

        auto* aligned = stream.Alloc(4u);
        REQUIRE(aligned == fixture.Block(0u).m_buffer.get() + 4u);

        stream.LoadDataInBlock(aligned, 4u);

        auto* alreadyAligned = stream.Alloc(4u);
        REQUIRE(alreadyAligned == fixture.Block(0u).m_buffer.get() + 8u);

        stream.PopBlock();
    }

    TEST_CASE("ZoneInputStream: a string is read up to and including its terminator", "[zoneloading]")
    {
        StreamFixture fixture(32u, 4u, 28u, {'a', 'b', 'c', '\0', 'd', '\0'});
        auto& stream = fixture.Stream();

        stream.PushBlock(0u);

        auto* first = stream.Alloc(1u);
        stream.LoadNullTerminated(first);
        REQUIRE(std::string(static_cast<const char*>(first)) == "abc");

        auto* second = stream.Alloc(1u);
        REQUIRE(second == fixture.Block(0u).m_buffer.get() + 4u);

        stream.LoadNullTerminated(second);
        REQUIRE(std::string(static_cast<const char*>(second)) == "d");

        stream.PopBlock();
    }

    TEST_CASE("ZoneInputStream: an unterminated string does not run past the stream", "[zoneloading]")
    {
        StreamFixture fixture(32u, 4u, 28u, {'a', 'b', 'c'});
        auto& stream = fixture.Stream();

        stream.PushBlock(0u);

        auto* buffer = stream.Alloc(1u);
        REQUIRE_THROWS(stream.LoadNullTerminated(buffer));

        stream.PopBlock();
    }

    TEST_CASE("ZoneInputStream: block usage is reported against the reserved size", "[zoneloading]")
    {
        StreamFixture fixture(32u, 4u, 28u, std::vector<uint8_t>(0x1000u, 0u), 0x10u);
        auto& stream = fixture.Stream();

        REQUIRE_FALSE(stream.ReportBlockUsage());

        for (auto i = 0u; i < StreamFixture::BLOCK_COUNT; i++)
        {
            stream.PushBlock(i);
            auto* buffer = stream.Alloc(1u);
            stream.LoadDataInBlock(buffer, 0x10u);
            stream.PopBlock();
        }

        REQUIRE(stream.ReportBlockUsage());
    }
} // namespace
