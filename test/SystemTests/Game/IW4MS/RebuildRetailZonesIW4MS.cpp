#ifdef ARCH_x64

#include "Game/IGame.h"
#include "OatTestPaths.h"
#include "Utils/Logging/Log.h"
#include "Utils/StringUtils.h"
#include "ZoneLoading.h"
#include "ZoneWriting.h"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

namespace
{
    void EnsureCanRebuild(const fs::path& outputPath, const fs::path& fastFilePath)
    {
        const auto fastFilePathStr = fastFilePath.string();
        const auto tempFilePath = outputPath / "temp.ff";
        const auto tempFilePathStr = tempFilePath.string();

        con::info("Testing IW4MS rebuild: {}", fastFilePathStr);

        auto maybeZone = ZoneLoading::LoadZone(fastFilePathStr, std::nullopt, GameId::IW4MS);
        REQUIRE(maybeZone.has_value());

        auto zone = std::move(*maybeZone);
        zone->m_name = "temp";

        {
            std::ofstream outStream(tempFilePath, std::ios::out | std::ios::binary);
            REQUIRE(outStream.is_open());
            REQUIRE(ZoneWriting::WriteZone(outStream, *zone));
        }

        auto maybeRebuiltZone = ZoneLoading::LoadZone(tempFilePathStr, std::nullopt, GameId::IW4MS);
        REQUIRE(maybeRebuiltZone.has_value());

        const auto rebuiltZone = std::move(*maybeRebuiltZone);

        const auto& pools = zone->m_pools;
        const auto& rebuiltPools = rebuiltZone->m_pools;

        REQUIRE(pools.GetTotalAssetCount() == rebuiltPools.GetTotalAssetCount());
        REQUIRE(zone->m_script_strings.Count() == rebuiltZone->m_script_strings.Count());

        auto zoneIter = pools.begin();
        const auto zoneEnd = pools.end();
        auto rebuiltIter = rebuiltPools.begin();
        const auto rebuiltEnd = rebuiltPools.end();

        while (zoneIter != zoneEnd)
        {
            REQUIRE(rebuiltIter != rebuiltEnd);
            REQUIRE((*zoneIter)->m_type == (*rebuiltIter)->m_type);
            REQUIRE((*zoneIter)->m_name == (*rebuiltIter)->m_name);

            ++zoneIter;
            ++rebuiltIter;
        }

        REQUIRE(rebuiltIter == rebuiltEnd);
    }

    TEST_CASE("IW4MS: retail zones survive a write and reload", "[iw4ms][custom]")
    {
        const auto* zoneDir = std::getenv("OAT_IW4MS_ZONE_DIR");
        if (!zoneDir)
            return;

        const auto outputPath = oat::paths::GetTempDirectory("RebuildRetailZonesIW4MS");
        const fs::path zonePath(zoneDir);

        for (const auto& entry : fs::directory_iterator(zonePath))
        {
            if (!entry.is_regular_file())
                continue;

            auto extension = entry.path().extension().string();
            utils::MakeStringLowerCase(extension);
            if (extension == ".ff")
                EnsureCanRebuild(outputPath, entry.path());
        }
    }
} // namespace

#endif // ARCH_x64
