#ifdef ARCH_x64

#include "ContentLoaderIW4MS.h"

#include "Game/IW4MS/AssetLoaderIW4MS.h"
#include "Game/IW4MS/IW4MS.h"
#include "Loading/Exception/UnsupportedAssetTypeException.h"
#include "Zone/Stream/ZoneStreamTrace.h"

#include <cassert>

using namespace IW4MS;

static_assert(sizeof(void*) == 8u, "IW4MS zones can only be loaded by a 64 bit build");

ContentLoader::ContentLoader(Zone& zone, ZoneInputStream& stream)
    : ContentLoaderBase(zone, stream),
      varXAssetList(nullptr),
      varXAsset(nullptr),
      varScriptStringList(nullptr)
{
}

void ContentLoader::LoadScriptStringList(const bool atStreamStart)
{
    assert(!atStreamStart);

    if (varScriptStringList->strings != nullptr)
    {
        assert(GetZonePointerType(varScriptStringList->strings) == ZonePointerType::FOLLOWING);

        // The pointer array is aligned to 4, not to its own 8 byte alignment. Load_ScriptStringList
        // reaches it through DB_AllocStreamPos(3) in the retail loader.
        varScriptStringList->strings = m_stream.Alloc<const char*>(4);
        varXString = varScriptStringList->strings;
        LoadXStringArray(true, varScriptStringList->count);

        if (varScriptStringList->strings && varScriptStringList->count > 0)
            m_zone.m_script_strings.InitializeForExistingZone(varScriptStringList->strings, static_cast<size_t>(varScriptStringList->count));
    }

    assert(m_zone.m_script_strings.Count() <= SCR_STRING_MAX + 1);
}

void ContentLoader::LoadXAsset(const bool atStreamStart) const
{
#define LOAD_ASSET(type_index, typeName, headerEntry)                                                                                                          \
    case type_index:                                                                                                                                           \
    {                                                                                                                                                          \
        Loader_##typeName loader(m_zone, m_stream);                                                                                                            \
        loader.Load(&varXAsset->header.headerEntry);                                                                                                           \
        break;                                                                                                                                                 \
    }
#define SKIP_ASSET(type_index, typeName, headerEntry)                                                                                                          \
    case type_index:                                                                                                                                           \
        break;

    assert(varXAsset != nullptr);

    if (atStreamStart)
        m_stream.Load<XAsset>(varXAsset);

    // Recorded before the load so that a trace ending mid asset still names the asset that failed.
    zone_trace::Record(zone_trace::op::ASSET,
                       -1,
                       0u,
                       0u,
                       static_cast<std::uint64_t>(varXAsset->type),
                       IGame::GetGameById(GameId::IW4MS)->GetAssetTypeName(varXAsset->type).value_or("?"));

    switch (varXAsset->type)
    {
        LOAD_ASSET(ASSET_TYPE_PHYSPRESET, PhysPreset, physPreset)
        LOAD_ASSET(ASSET_TYPE_PHYSCOLLMAP, PhysCollmap, physCollmap)
        LOAD_ASSET(ASSET_TYPE_XANIMPARTS, XAnimParts, parts)
        LOAD_ASSET(ASSET_TYPE_XMODEL, XModel, model)
        LOAD_ASSET(ASSET_TYPE_MATERIAL, Material, material)
        LOAD_ASSET(ASSET_TYPE_PIXELSHADER, MaterialPixelShader, pixelShader)
        LOAD_ASSET(ASSET_TYPE_VERTEXSHADER, MaterialVertexShader, vertexShader)
        LOAD_ASSET(ASSET_TYPE_VERTEXDECL, MaterialVertexDeclaration, vertexDecl)
        LOAD_ASSET(ASSET_TYPE_TECHNIQUE_SET, MaterialTechniqueSet, techniqueSet)
        LOAD_ASSET(ASSET_TYPE_IMAGE, GfxImage, image)
        LOAD_ASSET(ASSET_TYPE_SOUND, snd_alias_list_t, sound)
        LOAD_ASSET(ASSET_TYPE_SOUND_CURVE, SndCurve, sndCurve)
        LOAD_ASSET(ASSET_TYPE_LOADED_SOUND, LoadedSound, loadSnd)
        LOAD_ASSET(ASSET_TYPE_CLIPMAP_SP, clipMap_t, clipMap)
        LOAD_ASSET(ASSET_TYPE_CLIPMAP_MP, clipMap_t, clipMap)
        LOAD_ASSET(ASSET_TYPE_COMWORLD, ComWorld, comWorld)
        LOAD_ASSET(ASSET_TYPE_GAMEWORLD_SP, GameWorldSp, gameWorldSp)
        LOAD_ASSET(ASSET_TYPE_GAMEWORLD_MP, GameWorldMp, gameWorldMp)
        LOAD_ASSET(ASSET_TYPE_MAP_ENTS, MapEnts, mapEnts)
        LOAD_ASSET(ASSET_TYPE_FXWORLD, FxWorld, fxWorld)
        LOAD_ASSET(ASSET_TYPE_GFXWORLD, GfxWorld, gfxWorld)
        LOAD_ASSET(ASSET_TYPE_LIGHT_DEF, GfxLightDef, lightDef)
        LOAD_ASSET(ASSET_TYPE_FONT, Font_s, font)
        LOAD_ASSET(ASSET_TYPE_MENULIST, MenuList, menuList)
        LOAD_ASSET(ASSET_TYPE_MENU, menuDef_t, menu)
        LOAD_ASSET(ASSET_TYPE_LOCALIZE_ENTRY, LocalizeEntry, localize)
        LOAD_ASSET(ASSET_TYPE_WEAPON, WeaponCompleteDef, weapon)
        SKIP_ASSET(ASSET_TYPE_SNDDRIVER_GLOBALS, SndDriverGlobals, sndDriverGlobals)
        LOAD_ASSET(ASSET_TYPE_FX, FxEffectDef, fx)
        LOAD_ASSET(ASSET_TYPE_IMPACT_FX, FxImpactTable, impactFx)
        LOAD_ASSET(ASSET_TYPE_RAWFILE, RawFile, rawfile)
        LOAD_ASSET(ASSET_TYPE_STRINGTABLE, StringTable, stringTable)
        LOAD_ASSET(ASSET_TYPE_LEADERBOARD, LeaderboardDef, leaderboardDef)
        LOAD_ASSET(ASSET_TYPE_STRUCTURED_DATA_DEF, StructuredDataDefSet, structuredDataDefSet)
        LOAD_ASSET(ASSET_TYPE_TRACER, TracerDef, tracerDef)
        LOAD_ASSET(ASSET_TYPE_VEHICLE, VehicleDef, vehDef)
        LOAD_ASSET(ASSET_TYPE_ADDON_MAP_ENTS, AddonMapEnts, addonMapEnts)

    default:
    {
        throw UnsupportedAssetTypeException(varXAsset->type);
    }
    }

#undef SKIP_ASSET
#undef LOAD_ASSET
}

void ContentLoader::LoadXAssetArray(const bool atStreamStart, const size_t count)
{
    assert(varXAsset != nullptr);

    if (atStreamStart)
        m_stream.Load<XAsset>(varXAsset, count);

    for (size_t index = 0; index < count; index++)
    {
        LoadXAsset(false);
        varXAsset++;
    }
}

void ContentLoader::Load()
{
    XAssetList assetList{};
    varXAssetList = &assetList;

    // 32 bytes: a 16 byte ScriptStringList followed by the asset count and the asset table pointer,
    // each 8 byte aligned. Read raw because the root itself lives outside any block.
    m_stream.LoadDataRaw(&assetList, sizeof(assetList));

    m_stream.PushBlock(XFILE_BLOCK_VIRTUAL);

    varScriptStringList = &assetList.stringList;
    LoadScriptStringList(false);

    if (assetList.assets != nullptr)
    {
        assert(GetZonePointerType(assetList.assets) == ZonePointerType::FOLLOWING);

        // Aligned to 4 rather than to the structure's own alignment, matching DB_AllocStreamPos(3).
        assetList.assets = m_stream.Alloc<XAsset>(4);
        varXAsset = assetList.assets;
        LoadXAssetArray(true, assetList.assetCount);
    }

    m_stream.PopBlock();
}

#endif // ARCH_x64
