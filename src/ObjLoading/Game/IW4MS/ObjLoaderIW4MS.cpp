#include "ObjLoaderIW4MS.h"

#include "Utils/Logging/Log.h"

#ifdef ARCH_x64
#include "Asset/GlobalAssetPoolsLoader.h"
#include "Game/IW4MS/AssetMarkerIW4MS.h"
#include "Game/IW4MS/Font/FontLoaderIW4MS.h"
#include "Game/IW4MS/GameIW4MS.h"
#include "Game/IW4MS/IW4MS.h"
#include "Game/IW4MS/Image/ImageLoaderEmbeddedIW4MS.h"
#include "Game/IW4MS/Image/ImageLoaderExternalIW4MS.h"
#include "Game/IW4MS/Techset/PixelShaderLoaderIW4MS.h"
#include "Game/IW4MS/Techset/VertexShaderLoaderIW4MS.h"
#include "Game/IW4MS/Tracer/GdtLoaderTracerIW4MS.h"
#include "Game/IW4MS/Tracer/RawLoaderTracerIW4MS.h"
#include "Game/IW4MS/Vehicle/GdtLoaderVehicleIW4MS.h"
#include "Game/IW4MS/Vehicle/RawLoaderVehicleIW4MS.h"
#include "Game/IW4MS/Weapon/AccuracyGraphLoaderIW4MS.h"
#include "Game/IW4MS/XAnim/XAnimLoaderIW4MS.h"
#include "Game/IW4MS/XModel/LoaderXModelIW4MS.h"
#include "Leaderboard/LoaderLeaderboardIW4MS.h"
#include "LightDef/LightDefLoaderIW4MS.h"
#include "Localize/LoaderLocalizeIW4MS.h"
#include "Material/LoaderMaterialIW4MS.h"
#include "Menu/LoaderMenuListIW4MS.h"
#include "ObjLoading.h"
#include "PhysPreset/GdtLoaderPhysPresetIW4MS.h"
#include "PhysPreset/RawLoaderPhysPresetIW4MS.h"
#include "RawFile/LoaderRawFileIW4MS.h"
#include "Sound/LoaderSoundCurveIW4MS.h"
#include "StringTable/LoaderStringTableIW4MS.h"
#include "StructuredDataDef/LoaderStructuredDataDefIW4MS.h"
#include "Weapon/GdtLoaderWeaponIW4MS.h"
#include "Weapon/RawLoaderWeaponIW4MS.h"

#include <memory>
#endif // ARCH_x64

using namespace IW4MS;

void ObjLoader::LoadReferencedContainersForZone([[maybe_unused]] ISearchPath& searchPath, [[maybe_unused]] Zone& zone) const
{
}

void ObjLoader::UnloadContainersOfZone([[maybe_unused]] Zone& zone) const {}

#ifdef ARCH_x64
namespace
{
    void ConfigureDefaultCreators(AssetCreatorCollection& collection, Zone& zone)
    {
        auto& memory = zone.Memory();

        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetPhysPreset>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetPhysCollMap>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetXAnim>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetXModelSurfs>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetXModel>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetMaterial>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetPixelShader>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetVertexShader>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetVertexDecl>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetTechniqueSet>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetImage>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetSound>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetSoundCurve>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetLoadedSound>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetClipMapSp>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetClipMapMp>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetComWorld>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetGameWorldSp>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetGameWorldMp>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetMapEnts>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetFxWorld>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetGfxWorld>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetLightDef>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetFont>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetMenuList>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetMenu>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetLocalize>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetWeapon>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetFx>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetImpactFx>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetRawFile>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetStringTable>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetLeaderboard>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetStructuredDataDef>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetTracer>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetVehicle>>(memory));
        collection.AddDefaultAssetCreator(std::make_unique<DefaultAssetCreator<AssetAddonMapEnts>>(memory));
    }

    void ConfigureLoaders(AssetCreatorCollection& collection, Zone& zone, ISearchPath& searchPath, IGdtQueryable& gdt)
    {
        auto& memory = zone.Memory();

        collection.AddAssetCreator(phys_preset::CreateRawLoaderIW4MS(memory, searchPath, zone));
        collection.AddAssetCreator(phys_preset::CreateGdtLoaderIW4MS(memory, gdt, zone));
        collection.AddAssetCreator(xanim::CreateLoaderIW4MS(memory, searchPath, zone));
        collection.AddAssetCreator(xmodel::CreateLoaderIW4MS(memory, searchPath, zone));
        collection.AddAssetCreator(techset::CreateVertexShaderLoaderIW4MS(memory, searchPath));
        collection.AddAssetCreator(techset::CreatePixelShaderLoaderIW4MS(memory, searchPath));
        collection.AddAssetCreator(image::CreateLoaderEmbeddedIW4MS(memory, searchPath));
        collection.AddAssetCreator(image::CreateLoaderExternalIW4MS(memory, searchPath));
        collection.AddAssetCreator(font::CreateLoaderIW4MS(memory, searchPath));
        collection.AddAssetCreator(tracer::CreateRawLoaderIW4MS(memory, searchPath, zone));
        collection.AddAssetCreator(tracer::CreateGdtLoaderIW4MS(memory, gdt, zone));
        collection.AddAssetCreator(vehicle::CreateRawLoaderIW4MS(memory, searchPath, zone));
        collection.AddAssetCreator(vehicle::CreateGdtLoaderIW4MS(memory, gdt, zone));

        collection.AddAssetCreator(material::CreateLoaderIW4MS(memory, searchPath));
        collection.AddAssetCreator(sound_curve::CreateLoaderIW4MS(memory, searchPath));
        collection.AddAssetCreator(light_def::CreateLoaderIW4MS(memory, searchPath));
        collection.AddAssetCreator(menu::CreateMenuListLoaderIW4MS(memory, searchPath));
        collection.AddAssetCreator(localize::CreateLoaderIW4MS(memory, searchPath, zone));
        collection.AddAssetCreator(weapon::CreateRawLoaderIW4MS(memory, searchPath, zone));
        collection.AddAssetCreator(weapon::CreateGdtLoaderIW4MS(memory, searchPath, gdt, zone));
        collection.AddAssetCreator(raw_file::CreateLoaderIW4MS(memory, searchPath));
        collection.AddAssetCreator(string_table::CreateLoaderIW4MS(memory, searchPath));
        collection.AddAssetCreator(leaderboard::CreateLoaderIW4MS(memory, searchPath));
        collection.AddAssetCreator(structured_data_def::CreateLoaderIW4MS(memory, searchPath));

        collection.AddSubAssetCreator(weapon::CreateAccuracyGraphLoaderIW4MS(memory, searchPath));
    }

    void ConfigureGlobalAssetPoolsLoaders(AssetCreatorCollection& collection, Zone& zone)
    {
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetPhysPreset>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetPhysCollMap>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetXAnim>>(zone));
        // AssetXModelSurfs has no AssetMarkerWrapper, so it cannot be pooled globally: reached through XModel, never named on its own.
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetXModel>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetMaterial>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetPixelShader>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetVertexShader>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetVertexDecl>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetTechniqueSet>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetImage>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetSound>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetSoundCurve>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetLoadedSound>>(zone));
        // AssetClipMapSp has no AssetMarkerWrapper, so it cannot be pooled globally: the SP clipmap has no marker (only the MP one does).
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetClipMapMp>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetComWorld>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetGameWorldSp>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetGameWorldMp>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetMapEnts>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetFxWorld>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetGfxWorld>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetLightDef>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetFont>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetMenuList>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetMenu>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetLocalize>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetWeapon>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetFx>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetImpactFx>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetRawFile>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetStringTable>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetLeaderboard>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetStructuredDataDef>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetTracer>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetVehicle>>(zone));
        collection.AddAssetCreator(std::make_unique<GlobalAssetPoolsLoader<AssetAddonMapEnts>>(zone));
    }
} // namespace

namespace IW4MS
{
    void ObjLoader::ConfigureCreatorCollection(AssetCreatorCollection& collection, Zone& zone, ISearchPath& searchPath, IGdtQueryable& gdt) const
    {
        ConfigureDefaultCreators(collection, zone);
        ConfigureLoaders(collection, zone, searchPath, gdt);
        ConfigureGlobalAssetPoolsLoaders(collection, zone);
    }
} // namespace IW4MS
#else
namespace IW4MS
{
    void ObjLoader::ConfigureCreatorCollection([[maybe_unused]] AssetCreatorCollection& collection,
                                               [[maybe_unused]] Zone& zone,
                                               [[maybe_unused]] ISearchPath& searchPath,
                                               [[maybe_unused]] IGdtQueryable& gdt) const
    {
        con::error("Loading IW4MS assets from source needs a 64 bit build: its pointers are 8 bytes wide.");
    }
} // namespace IW4MS
#endif // ARCH_x64
