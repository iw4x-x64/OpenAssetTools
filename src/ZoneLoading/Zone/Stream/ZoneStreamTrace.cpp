#include "ZoneStreamTrace.h"

#include <format>
#include <fstream>

namespace zone_trace
{
    ISink* g_sink = nullptr;

    void SetSink(ISink* sink)
    {
        g_sink = sink;
    }

    namespace
    {
        class FileSink final : public ISink
        {
        public:
            explicit FileSink(const std::string& path)
                : m_stream(path, std::ios::out | std::ios::trunc),
                  m_sequence(0u)
            {
            }

            [[nodiscard]] bool IsOpen() const
            {
                return m_stream.is_open();
            }

            void Record(const std::string_view operation,
                        const int block,
                        const size_t offsetBefore,
                        const size_t offsetAfter,
                        const std::uint64_t detail,
                        const std::string_view text) override
            {
                // Fixed width fields keep records aligned so a diff points at a column, and the
                // leading sequence number makes "first divergence" a head -1 away.
                m_stream << std::format("{:08}\t{}\t{}\t{:08x}\t{:08x}\t{:016x}\t{}\n",
                                        m_sequence++,
                                        operation,
                                        block,
                                        offsetBefore,
                                        offsetAfter,
                                        detail,
                                        text);
            }

        private:
            std::ofstream m_stream;
            size_t m_sequence;
        };
    } // namespace

    std::unique_ptr<ISink> CreateFileSink(const std::string& path)
    {
        auto sink = std::make_unique<FileSink>(path);
        if (!sink->IsOpen())
            return nullptr;

        return sink;
    }
} // namespace zone_trace
