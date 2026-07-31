#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

/**
 * \brief Records every operation a zone input stream performs on its blocks.
 *
 * Reverse-engineering a serialized format is mostly a search for the first read that goes to the
 * wrong place. Once the stream cursor is off by even one byte, every pointer after it resolves into
 * the wrong block memory, and the symptom shows up as a corrupt asset far from the cause.
 *
 * A trace turns that search into a diff: run two implementations, or one implementation before and
 * after a change, and the first differing line is the operation that broke. Records therefore carry
 * a monotonic sequence number and no timestamps, addresses or other run-varying data.
 */
namespace zone_trace
{
    /** Kinds of stream operation. Kept short because they appear in every record. */
    namespace op
    {
        inline constexpr const char* ALIGN = "align";
        inline constexpr const char* READ = "read";
        inline constexpr const char* MEMSET = "memset";
        inline constexpr const char* STRING = "str";
        inline constexpr const char* PUSH = "push";
        inline constexpr const char* POP = "pop";
        inline constexpr const char* ALLOC = "alloc";
        inline constexpr const char* ALLOC_OOB = "allocoob";
        inline constexpr const char* POINTER = "ptr";
        inline constexpr const char* INSERT = "insert";
        inline constexpr const char* ASSET = "asset";
    } // namespace op

    class ISink
    {
    public:
        virtual ~ISink() = default;
        ISink() = default;
        ISink(const ISink& other) = default;
        ISink(ISink&& other) noexcept = default;
        ISink& operator=(const ISink& other) = default;
        ISink& operator=(ISink&& other) noexcept = default;

        /**
         * \param operation One of the op:: constants.
         * \param block Index of the block the operation applies to, or -1 when none is active.
         * \param offsetBefore Block cursor before the operation.
         * \param offsetAfter Block cursor after the operation.
         * \param detail Operation specific number: byte count, alignment, or a raw serialized pointer.
         * \param text Operation specific text: an asset or string value. May be empty.
         */
        virtual void Record(std::string_view operation,
                            int block,
                            size_t offsetBefore,
                            size_t offsetAfter,
                            std::uint64_t detail,
                            std::string_view text) = 0;
    };

    /**
     * \brief The active sink, or nullptr when tracing is off.
     *
     * Exposed directly so the hot paths can skip the call with a single predictable branch. A zone
     * of any size performs millions of stream operations, so this must stay cheap when disabled.
     */
    extern ISink* g_sink;

    [[nodiscard]] inline bool Enabled()
    {
        return g_sink != nullptr;
    }

    inline void Record(const std::string_view operation,
                       const int block,
                       const size_t offsetBefore,
                       const size_t offsetAfter,
                       const std::uint64_t detail = 0u,
                       const std::string_view text = {})
    {
        if (g_sink)
            g_sink->Record(operation, block, offsetBefore, offsetAfter, detail, text);
    }

    void SetSink(ISink* sink);

    /** Creates a sink writing the machine readable format to a file. Returns nullptr if it cannot be opened. */
    std::unique_ptr<ISink> CreateFileSink(const std::string& path);
} // namespace zone_trace
