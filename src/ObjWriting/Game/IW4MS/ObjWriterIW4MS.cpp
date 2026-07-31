#include "ObjWriterIW4MS.h"

#include "Game/IW4MS/FX/EfxDumperIW4MS.h"
#include "Game/IW4MS/FX/ImpactFxJsonDumperIW4MS.h"
#include "Game/IW4MS/Font/FontDumperIW4MS.h"
#include "Game/IW4MS/Image/ImageDumperIW4MS.h"
#include "Game/IW4MS/Maps/MapEntsDumperIW4MS.h"
#include "Game/IW4MS/Material/MaterialJsonDumperIW4MS.h"
#include "Game/IW4MS/Sound/SndAliasListCsvDumperIW4MS.h"
#include "Game/IW4MS/Techset/PixelShaderDumperIW4MS.h"
#include "Game/IW4MS/Techset/TechsetDumperIW4MS.h"
#include "Game/IW4MS/Techset/VertexDeclJsonDumperIW4MS.h"
#include "Game/IW4MS/Techset/VertexShaderDumperIW4MS.h"
#include "Game/IW4MS/Tracer/TracerDumperIW4MS.h"
#include "Game/IW4MS/Vehicle/VehicleDumperIW4MS.h"
#include "Game/IW4MS/World/ClipMapJsonDumperIW4MS.h"
#include "Game/IW4MS/World/ComWorldJsonDumperIW4MS.h"
#include "Game/IW4MS/World/FxWorldJsonDumperIW4MS.h"
#include "Game/IW4MS/World/GameWorldJsonDumperIW4MS.h"
#include "Game/IW4MS/World/GfxWorldJsonDumperIW4MS.h"
#include "Game/IW4MS/XAnim/XAnimDumperIW4MS.h"
#include "Game/IW4MS/XModel/XModelDumperIW4MS.h"
#include "Leaderboard/LeaderboardJsonDumperIW4MS.h"
#include "LightDef/LightDefDumperIW4MS.h"
#include "Localize/LocalizeDumperIW4MS.h"
#include "Maps/AddonMapEntsDumperIW4MS.h"
#include "Menu/MenuDumperIW4MS.h"
#include "Menu/MenuListDumperIW4MS.h"
#include "PhysCollmap/PhysCollmapDumperIW4MS.h"
#include "PhysPreset/PhysPresetInfoStringDumperIW4MS.h"
#include "RawFile/RawFileDumperIW4MS.h"
#include "Sound/LoadedSoundDumperIW4MS.h"
#include "Sound/SndCurveDumperIW4MS.h"
#include "StringTable/StringTableDumperIW4MS.h"
#include "StructuredDataDef/StructuredDataDefDumperIW4MS.h"
#include "Weapon/WeaponDumperIW4MS.h"

using namespace IW4MS;

void ObjWriter::RegisterAssetDumpers([[maybe_unused]] AssetDumpingContext& context)
{
    RegisterAssetDumper(std::make_unique<phys_preset::InfoStringDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<fx::EfxDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<fx::ImpactFxJsonDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<techset::VertexDeclJsonDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<world::ComWorldJsonDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<world::FxWorldJsonDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<world::GameWorldSpJsonDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<world::GameWorldMpJsonDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<world::ClipMapSpJsonDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<world::ClipMapMpJsonDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<world::GfxWorldJsonDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<phys_collmap::DumperIW4MS>());
    RegisterAssetDumper(std::make_unique<xanim::DumperIW4MS>());
    RegisterAssetDumper(std::make_unique<xmodel::DumperIW4MS>());
    RegisterAssetDumper(std::make_unique<material::JsonDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<techset::PixelShaderDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<techset::VertexShaderDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<techset::DumperIW4MS>(false));
    RegisterAssetDumper(std::make_unique<image::DumperIW4MS>());
    RegisterAssetDumper(std::make_unique<sound::CsvDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<sound_curve::DumperIW4MS>());
    RegisterAssetDumper(std::make_unique<sound::LoadedSoundDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<map_ents::DumperIW4MS>());
    RegisterAssetDumper(std::make_unique<light_def::DumperIW4MS>());
    RegisterAssetDumper(std::make_unique<font::JsonDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<menu::MenuListDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<menu::MenuDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<localize::DumperIW4MS>());
    RegisterAssetDumper(std::make_unique<weapon::DumperIW4MS>());
    RegisterAssetDumper(std::make_unique<raw_file::DumperIW4MS>());
    RegisterAssetDumper(std::make_unique<string_table::DumperIW4MS>());
    RegisterAssetDumper(std::make_unique<leaderboard::JsonDumperIW4MS>());
    RegisterAssetDumper(std::make_unique<structured_data_def::DumperIW4MS>());
    RegisterAssetDumper(std::make_unique<tracer::DumperIW4MS>());
    RegisterAssetDumper(std::make_unique<vehicle::DumperIW4MS>());
    RegisterAssetDumper(std::make_unique<addon_map_ents::DumperIW4MS>());
}
