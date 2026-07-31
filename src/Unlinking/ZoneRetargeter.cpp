#include "ZoneRetargeter.h"

#include "Game/IW4/IW4.h"
#include "Game/IW4MS/IW4MS.h"
#include "Pool/XAssetInfo.h"
#include "Utils/Logging/Log.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <limits>
#include <type_traits>
#include <unordered_map>

namespace
{
#ifdef ARCH_x64
    void* RetargetClipMap(const void* source, ZoneMemory& memory)
    {
        static_assert(sizeof(IW4::clipMap_t) < sizeof(IW4MS::clipMap_t));
        static_assert(offsetof(IW4::clipMap_t, checksum) == offsetof(IW4MS::clipMap_t, checksum));

        auto* target = memory.Alloc<IW4MS::clipMap_t>();
        std::memcpy(target, source, sizeof(IW4::clipMap_t));
        std::memset(reinterpret_cast<char*>(target) + sizeof(IW4::clipMap_t), 0, sizeof(IW4MS::clipMap_t) - sizeof(IW4::clipMap_t));

        return target;
    }

    IW4MS::SpeakerMap* RetargetSpeakerMap(const IW4::SpeakerMap& source, ZoneMemory& memory)
    {
        auto* target = memory.Alloc<IW4MS::SpeakerMap>();
        target->isDefault = source.isDefault;
        target->name = source.name;

        for (auto channelCount = 0u; channelCount < 2u; channelCount++)
        {
            for (auto speakerConfig = 0u; speakerConfig < 2u; speakerConfig++)
            {
                const auto& sourceMap = source.channelMaps[channelCount][speakerConfig];
                auto& targetLevels = target->channelMaps[channelCount].speakers[speakerConfig];

                targetLevels.levelCount = 0u;
                std::memset(targetLevels.unknown_1, 0, sizeof(targetLevels.unknown_1));
                targetLevels.levels = nullptr;

                auto cellCount = 0u;
                for (auto i = 0; i < sourceMap.speakerCount && i < static_cast<int>(std::extent_v<decltype(sourceMap.speakers)>); i++)
                    cellCount += static_cast<unsigned>(std::max(0, sourceMap.speakers[i].numLevels));

                if (cellCount == 0u)
                    continue;

                assert(cellCount <= std::numeric_limits<unsigned char>::max());

                auto* cells = memory.Alloc<IW4MS::MSSSpeakerLevel>(cellCount);
                auto cell = 0u;

                for (auto i = 0; i < sourceMap.speakerCount && i < static_cast<int>(std::extent_v<decltype(sourceMap.speakers)>); i++)
                {
                    const auto& speaker = sourceMap.speakers[i];

                    for (auto channel = 0; channel < speaker.numLevels && channel < static_cast<int>(std::extent_v<decltype(speaker.levels)>); channel++)
                    {
                        cells[cell].channel = static_cast<unsigned char>(channel);
                        cells[cell].speaker = static_cast<unsigned char>(speaker.speaker);
                        cells[cell].unused[0] = 0u;
                        cells[cell].unused[1] = 0u;
                        cells[cell].gain = speaker.levels[channel];
                        cell++;
                    }
                }

                targetLevels.levelCount = static_cast<unsigned char>(cellCount);
                targetLevels.levels = cells;
            }
        }

        return target;
    }

    constexpr auto IW4_GFX_AABB_TREE_SIZE = 44;
    constexpr auto IW4MS_GFX_AABB_TREE_SIZE = 56;

    bool RetargetGfxWorld(IW4::GfxWorld& world, unsigned& rescaledOffsets)
    {
        static_assert(sizeof(IW4MS::GfxAabbTree) == IW4MS_GFX_AABB_TREE_SIZE);

        if (!world.aabbTrees || !world.aabbTreeCounts)
            return true;

        for (auto cell = 0; cell < world.dpvsPlanes.cellCount; cell++)
        {
            auto* trees = world.aabbTrees[cell].aabbTree;
            if (!trees)
                continue;

            for (auto i = 0; i < world.aabbTreeCounts[cell]; i++)
            {
                auto& offset = trees[i].childrenOffset;

                if (offset % IW4_GFX_AABB_TREE_SIZE != 0)
                {
                    if (trees[i].childCount == 0)
                        continue;

                    con::error("Cannot retarget this zone: aabb tree {} of cell {} has {} children at a childrenOffset "
                               "of {}, which is not a whole number of {} byte records and so cannot be restated in the "
                               "x64 stride.",
                               i,
                               cell,
                               trees[i].childCount,
                               offset,
                               IW4_GFX_AABB_TREE_SIZE);
                    return false;
                }

                offset = offset / IW4_GFX_AABB_TREE_SIZE * IW4MS_GFX_AABB_TREE_SIZE;
                rescaledOffsets++;
            }
        }

        return true;
    }

    std::unique_ptr<Zone> RetargetIw4ToIw4ms(const Zone& source)
    {
        static_assert(static_cast<int>(IW4::ASSET_TYPE_COUNT) == static_cast<int>(IW4MS::ASSET_TYPE_COUNT));
        static_assert(static_cast<int>(IW4::ASSET_TYPE_CLIPMAP_MP) == static_cast<int>(IW4MS::ASSET_TYPE_CLIPMAP_MP));
        static_assert(static_cast<int>(IW4::ASSET_TYPE_ADDON_MAP_ENTS) == static_cast<int>(IW4MS::ASSET_TYPE_ADDON_MAP_ENTS));

        auto target = std::make_unique<Zone>(source.m_name, source.m_priority, GameId::IW4MS, source.m_platform);
        target->m_language = source.m_language;

        for (auto i = 0u; i < source.m_script_strings.Count(); i++)
            target->m_script_strings.AddOrGetScriptString(source.m_script_strings.CValue(i));

        auto waterMaterials = 0u;
        auto speakerMaps = 0u;
        auto rescaledOffsets = 0u;
        std::unordered_map<const IW4::SpeakerMap*, IW4MS::SpeakerMap*> retargetedSpeakerMaps;

        for (const auto* asset : source.m_pools)
        {
            if (asset->m_type == IW4::ASSET_TYPE_MATERIAL)
            {
                const auto* material = static_cast<const IW4::Material*>(asset->m_ptr);
                for (auto i = 0u; i < material->textureCount; i++)
                {
                    if (material->textureTable && material->textureTable[i].semantic == IW4::TS_WATER_MAP && material->textureTable[i].u.water)
                        waterMaterials++;
                }
            }
            else if (asset->m_type == IW4::ASSET_TYPE_SOUND)
            {
                auto* soundList = static_cast<IW4::snd_alias_list_t*>(asset->m_ptr);
                for (auto i = 0; soundList->head && i < soundList->count; i++)
                {
                    auto& alias = soundList->head[i];
                    if (!alias.speakerMap)
                        continue;

                    const auto existing = retargetedSpeakerMaps.find(alias.speakerMap);
                    if (existing != retargetedSpeakerMaps.end())
                    {
                        alias.speakerMap = reinterpret_cast<IW4::SpeakerMap*>(existing->second);
                        continue;
                    }

                    auto* retargeted = RetargetSpeakerMap(*alias.speakerMap, target->Memory());
                    retargetedSpeakerMaps.emplace(alias.speakerMap, retargeted);
                    alias.speakerMap = reinterpret_cast<IW4::SpeakerMap*>(retargeted);
                    speakerMaps++;
                }
            }
            else if (asset->m_type == IW4::ASSET_TYPE_GFXWORLD)
            {
                if (!RetargetGfxWorld(*static_cast<IW4::GfxWorld*>(asset->m_ptr), rescaledOffsets))
                    return nullptr;
            }
        }

        if (waterMaterials > 0u)
        {
            con::error("Cannot retarget this zone: {} of its materials use a water texture, and water_t is laid out "
                       "differently in the x64 build in a way that is not yet resolved. See F-014 and Q-008.",
                       waterMaterials);
            return nullptr;
        }

        auto retargetedClipMaps = 0u;
        for (const auto* asset : source.m_pools)
        {
            auto* pointer = asset->m_ptr;

            if (asset->m_type == IW4::ASSET_TYPE_CLIPMAP_SP || asset->m_type == IW4::ASSET_TYPE_CLIPMAP_MP)
            {
                pointer = RetargetClipMap(pointer, target->Memory());
                retargetedClipMaps++;
            }

            target->m_pools.AddAsset(asset->m_type, asset->m_name, pointer, {}, asset->m_used_script_strings, {});
        }

        con::info("Retargeted {} assets from IW4 to IW4MS: {} clipmaps rebuilt for the wider struct, {} speaker maps "
                  "reshaped into the x64 mix matrix, {} aabb tree child offsets restated in the x64 stride",
                  source.m_pools.GetTotalAssetCount(),
                  retargetedClipMaps,
                  speakerMaps,
                  rescaledOffsets);

        return target;
    }
#endif
} // namespace

namespace retarget
{
    std::unique_ptr<Zone> Retarget([[maybe_unused]] const Zone& source, const GameId targetGame)
    {
#ifdef ARCH_x64
        if (source.m_game_id == GameId::IW4 && targetGame == GameId::IW4MS)
            return RetargetIw4ToIw4ms(source);

        con::error("Cannot retarget {} to {}: only IW4 to IW4MS is supported, since they are the one pair that is the "
                   "same game built for two word sizes.",
                   GameId_Names[static_cast<unsigned>(source.m_game_id)],
                   GameId_Names[static_cast<unsigned>(targetGame)]);
#else
        con::error("Retargeting to {} needs a 64 bit build.", GameId_Names[static_cast<unsigned>(targetGame)]);
#endif

        return nullptr;
    }
} // namespace retarget
