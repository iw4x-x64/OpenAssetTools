#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/IW4MS/IW4MS.h"

namespace menu
{
    class MenuDumperIW4MS final : public AbstractAssetDumper<IW4MS::AssetMenu>
    {
    protected:
        void DumpAsset(AssetDumpingContext& context, const XAssetInfo<IW4MS::AssetMenu::Type>& asset) override;
    };
} // namespace menu
