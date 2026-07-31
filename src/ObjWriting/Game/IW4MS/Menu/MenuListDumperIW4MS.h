#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/IW4MS/IW4MS.h"
#include "Menu/MenuDumpingZoneState.h"

namespace menu
{
    void CreateDumpingStateForMenuListIW4MS(MenuDumpingZoneState* zoneState, const IW4MS::MenuList* menuList);

    class MenuListDumperIW4MS final : public AbstractAssetDumper<IW4MS::AssetMenuList>
    {
    public:
        void Dump(AssetDumpingContext& context) override;

    protected:
        void DumpAsset(AssetDumpingContext& context, const XAssetInfo<IW4MS::AssetMenuList::Type>& asset) override;
    };
} // namespace menu
