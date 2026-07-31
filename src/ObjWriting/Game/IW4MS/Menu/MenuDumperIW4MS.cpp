#include "MenuDumperIW4MS.h"

#include "MenuListDumperIW4MS.h"
#include "MenuWriterIW4MS.h"
#include "ObjWriting.h"

#include <filesystem>
#include <string>

using namespace IW4MS;

namespace
{
    std::string GetPathForMenu(menu::MenuDumpingZoneState* zoneState, const XAssetInfo<menuDef_t>& asset)
    {
        const auto menuDumpingState = zoneState->m_menu_dumping_state_map.find(asset.Asset());

        if (menuDumpingState == zoneState->m_menu_dumping_state_map.end())
            return "ui_mp/" + std::string(asset.Asset()->window.name) + ".menu";

        return menuDumpingState->second.m_path;
    }
} // namespace

namespace menu
{
    void MenuDumperIW4MS::DumpAsset(AssetDumpingContext& context, const XAssetInfo<AssetMenu::Type>& asset)
    {
        const auto* menu = asset.Asset();
        auto* zoneState = context.GetZoneAssetDumperState<MenuDumpingZoneState>();

        if (!ObjWriting::ShouldHandleAssetType(ASSET_TYPE_MENULIST))
        {
            // Make sure menu paths based on menu lists are created
            auto menuListAssets = context.m_zone.m_pools.PoolAssets<AssetMenuList>();
            for (auto* menuListAsset : menuListAssets)
                CreateDumpingStateForMenuListIW4MS(zoneState, menuListAsset->Asset());
        }

        const auto menuFilePath = GetPathForMenu(zoneState, asset);
        const auto assetFile = context.OpenAssetFile(menuFilePath);

        if (!assetFile)
            return;

        auto menuWriter = CreateMenuWriterIW4MS(*assetFile);

        menuWriter->Start();
        menuWriter->WriteMenu(*menu);
        menuWriter->End();
    }
} // namespace menu
