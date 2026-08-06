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

    // x64 build splits x86's single interleaved complex_s H0 array into two
    // planar float arrays, and which half holds the real part follows from the
    // sim update.
    //
    // x64 sub_140038420 reads wTerm from +24, and the two planar arrays from +8
    // and +16:
    //
    //     movss xmm0, [rbx+rax*4]       ; wave table at the quarter period shifted index
    //     mulss xmm0, [r9]              ;   times the array at +8
    //     movss [r14+rdx*4], xmm0       ;   into the FFT real input
    //     movss xmm0, [rbx+rcx*4]       ; wave table at the unshifted index
    //     mulss xmm0, [r10]             ;   times the array at +16
    //     movss [r14+rdx*4+4000h], xmm0 ; into the FFT imaginary input
    //
    // x86 sub_427C80 is the same routine over the interleaved array, stepping
    // it 8 bytes a turn:
    //
    //     flds  0x6ba7f00(,%ebp,4)  ; same table, same quarter period shifted index
    //     fmuls (%edx)              ;   times H0[i].real
    //     fstps (%ebx,%ecx,8)       ;   into the FFT real input
    //     flds  0x6ba7f00(,%eax,4)  ; same table, unshifted
    //     fmuls 0x4(%edx)           ;   times H0[i].imag
    //     fstps 0x4(%ebx,%ecx,8)    ;   into the FFT imaginary input
    //     add   $0x8,%edx           ; one complex_s per iteration
    //
    // Both index the table by `floatTime * wTerm[i] * (1024 / 2pi)` masked to
    // 1024 entries, with the same 162.97466 constant. The array at +8 therefore
    // takes the place of H0[i].real and the array at +16 takes the place of
    // H0[i].imag, so H0Part0 is the real half and H0Part1 the imaginary one.
    //
    IW4MS::water_t* RetargetWater(const IW4::water_t& source, ZoneMemory& memory)
    {
        auto* target = memory.Alloc<IW4MS::water_t>();

        target->writable.floatTime = source.writable.floatTime;
        target->M = source.M;
        target->N = source.N;
        target->Lx = source.Lx;
        target->Lz = source.Lz;
        target->gravity = source.gravity;
        target->windvel = source.windvel;
        target->amplitude = source.amplitude;
        std::memcpy(target->winddir, source.winddir, sizeof(target->winddir));
        std::memcpy(target->codeConstant, source.codeConstant, sizeof(target->codeConstant));
        target->image = reinterpret_cast<IW4MS::GfxImage*>(source.image);

        // Load_water_t streams M * N floats for each of the three arrays, which
        // is what the zone code counts them as. wTerm keeps both its element
        // type and its length, so it carries over untouched and only H0 has to
        // be deinterleaved.
        //
        const auto cellCount = source.M > 0 && source.N > 0 ? static_cast<size_t>(source.M) * static_cast<size_t>(source.N) : 0u;

        target->wTerm = source.wTerm;
        target->H0Part0 = nullptr;
        target->H0Part1 = nullptr;

        if (source.H0 && cellCount > 0u)
        {
            target->H0Part0 = memory.Alloc<float>(cellCount);
            target->H0Part1 = memory.Alloc<float>(cellCount);

            for (auto i = 0u; i < cellCount; i++)
            {
                target->H0Part0[i] = source.H0[i].real;
                target->H0Part1[i] = source.H0[i].imag;
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

        // The assets keep the raw scr_string_t values they were loaded with, so
        // the target list has to come out index for index identical to the
        // source one. A fresh Zone starts with the placeholder null entry its
        // constructor seeds at index 0, and AddOrGetScriptString both collapses
        // duplicates and cannot reuse that placeholder, either of which shifts
        // every index and silently repoints every bone name and tag in the
        // zone. Initializing the list the way a loaded zone does drops the
        // placeholder, and appending verbatim keeps duplicates and the null
        // position where they were.
        //
        target->m_script_strings.InitializeForExistingZone();
        for (auto i = 0u; i < source.m_script_strings.Count(); i++)
            target->m_script_strings.AddScriptString(source.m_script_strings.CValue(i));

        auto waters = 0u;
        auto speakerMaps = 0u;
        auto rescaledOffsets = 0u;
        std::unordered_map<const IW4::SpeakerMap*, IW4MS::SpeakerMap*> retargetedSpeakerMaps;
        std::unordered_map<const IW4::water_t*, IW4MS::water_t*> retargetedWaters;

        for (const auto* asset : source.m_pools)
        {
            if (asset->m_type == IW4::ASSET_TYPE_MATERIAL)
            {
                auto* material = static_cast<IW4::Material*>(asset->m_ptr);
                if (!material->textureTable)
                    continue;

                for (auto i = 0u; i < material->textureCount; i++)
                {
                    auto& textureDef = material->textureTable[i];
                    if (textureDef.semantic != IW4::TS_WATER_MAP || !textureDef.u.water)
                        continue;

                    // The water is reusable, so materials sharing one must keep
                    // sharing it.
                    //
                    const auto existing = retargetedWaters.find(textureDef.u.water);
                    if (existing != retargetedWaters.end())
                    {
                        textureDef.u.water = reinterpret_cast<IW4::water_t*>(existing->second);
                        continue;
                    }

                    auto* retargeted = RetargetWater(*textureDef.u.water, target->Memory());
                    retargetedWaters.emplace(textureDef.u.water, retargeted);
                    textureDef.u.water = reinterpret_cast<IW4::water_t*>(retargeted);
                    waters++;
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
                  "reshaped into the x64 mix matrix, {} aabb tree child offsets restated in the x64 stride, {} waters "
                  "deinterleaved into planar spectrum halves",
                  source.m_pools.GetTotalAssetCount(),
                  retargetedClipMaps,
                  speakerMaps,
                  rescaledOffsets,
                  waters);

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
