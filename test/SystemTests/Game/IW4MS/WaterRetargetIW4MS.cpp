#ifdef ARCH_x64

#include "Game/IW4/IW4.h"
#include "Game/IW4MS/IW4MS.h"
#include "OatTestPaths.h"
#include "Utils/Logging/Log.h"
#include "ZoneLoading.h"
#include "ZoneRetargeter.h"
#include "ZoneWriting.h"

#include <catch2/catch_test_macros.hpp>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{
    struct WaterSnapshot
    {
        int m_m;
        int m_n;
        std::vector<float> m_real;
        std::vector<float> m_imag;
        std::vector<float> m_w_term;
    };

    using WaterSnapshots = std::map<std::pair<std::string, unsigned>, WaterSnapshot>;

    // Keyed by material name and texture index so the two zones can be matched
    // up without relying on the pools coming back in the same order.
    //
    WaterSnapshots SnapshotSourceWaters(const Zone& zone)
    {
        WaterSnapshots snapshots;

        for (const auto* asset : zone.m_pools)
        {
            if (asset->m_type != IW4::ASSET_TYPE_MATERIAL)
                continue;

            const auto* material = static_cast<const IW4::Material*>(asset->m_ptr);
            if (!material->textureTable)
                continue;

            for (auto i = 0u; i < material->textureCount; i++)
            {
                const auto& textureDef = material->textureTable[i];
                if (textureDef.semantic != IW4::TS_WATER_MAP || !textureDef.u.water)
                    continue;

                const auto& water = *textureDef.u.water;
                const auto cellCount = water.M > 0 && water.N > 0 ? static_cast<size_t>(water.M) * static_cast<size_t>(water.N) : 0u;

                WaterSnapshot snapshot;
                snapshot.m_m = water.M;
                snapshot.m_n = water.N;

                if (water.H0)
                {
                    snapshot.m_real.reserve(cellCount);
                    snapshot.m_imag.reserve(cellCount);

                    for (auto cell = 0u; cell < cellCount; cell++)
                    {
                        snapshot.m_real.push_back(water.H0[cell].real);
                        snapshot.m_imag.push_back(water.H0[cell].imag);
                    }
                }

                if (water.wTerm)
                    snapshot.m_w_term.assign(water.wTerm, water.wTerm + cellCount);

                snapshots.emplace(std::make_pair(asset->m_name, i), std::move(snapshot));
            }
        }

        return snapshots;
    }

    void CompareRebuiltWaters(const Zone& zone, const WaterSnapshots& expected)
    {
        auto compared = 0u;

        for (const auto* asset : zone.m_pools)
        {
            if (asset->m_type != IW4MS::ASSET_TYPE_MATERIAL)
                continue;

            const auto* material = static_cast<const IW4MS::Material*>(asset->m_ptr);
            if (!material->textureTable)
                continue;

            for (auto i = 0u; i < material->textureCount; i++)
            {
                const auto& textureDef = material->textureTable[i];
                if (textureDef.semantic != IW4MS::TS_WATER_MAP || !textureDef.u.water)
                    continue;

                const auto entry = expected.find(std::make_pair(asset->m_name, i));
                REQUIRE(entry != expected.end());

                const auto& snapshot = entry->second;
                const auto& water = *textureDef.u.water;

                REQUIRE(water.M == snapshot.m_m);
                REQUIRE(water.N == snapshot.m_n);

                REQUIRE((water.H0Part0 != nullptr) == !snapshot.m_real.empty());
                REQUIRE((water.H0Part1 != nullptr) == !snapshot.m_imag.empty());
                REQUIRE((water.wTerm != nullptr) == !snapshot.m_w_term.empty());

                // H0Part0 is the real half and H0Part1 the imaginary one,
                // established from the sim update in both builds. See the note
                // on RetargetWater.
                //
                for (auto cell = 0u; cell < snapshot.m_real.size(); cell++)
                {
                    REQUIRE(water.H0Part0[cell] == snapshot.m_real[cell]);
                    REQUIRE(water.H0Part1[cell] == snapshot.m_imag[cell]);
                }

                for (auto cell = 0u; cell < snapshot.m_w_term.size(); cell++)
                    REQUIRE(water.wTerm[cell] == snapshot.m_w_term[cell]);

                compared++;
            }
        }

        REQUIRE(compared == expected.size());
    }

    TEST_CASE("IW4MS: retargeting deinterleaves water spectra intact", "[iw4ms][water][custom]")
    {
        const auto* zoneFile = std::getenv("OAT_IW4_WATER_ZONE");
        if (!zoneFile)
            return;

        const auto outputPath = oat::paths::GetTempDirectory("WaterRetargetIW4MS");
        const auto tempFilePath = outputPath / "water.ff";

        auto maybeSource = ZoneLoading::LoadZone(zoneFile, std::nullopt, GameId::IW4);
        REQUIRE(maybeSource.has_value());

        const auto source = std::move(*maybeSource);

        // Retargeting repoints the source materials at the new waters, so take
        // the reference copy of the x86 data first.
        //
        const auto snapshots = SnapshotSourceWaters(*source);
        REQUIRE(!snapshots.empty());
        con::info("Snapshotted {} waters from {}", snapshots.size(), zoneFile);

        auto retargeted = retarget::Retarget(*source, GameId::IW4MS);
        REQUIRE(retargeted != nullptr);
        retargeted->m_name = "water";

        {
            std::ofstream outStream(tempFilePath, std::ios::out | std::ios::binary);
            REQUIRE(outStream.is_open());
            REQUIRE(ZoneWriting::WriteZone(outStream, *retargeted));
        }

        auto maybeRebuilt = ZoneLoading::LoadZone(tempFilePath.string(), std::nullopt, GameId::IW4MS);
        REQUIRE(maybeRebuilt.has_value());

        CompareRebuiltWaters(**maybeRebuilt, snapshots);
    }
} // namespace

#endif // ARCH_x64
