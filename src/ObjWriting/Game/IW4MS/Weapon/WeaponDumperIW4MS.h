#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/IW4MS/IW4MS.h"

namespace weapon
{
    class DumperIW4MS final : public AbstractAssetDumper<IW4MS::AssetWeapon>
    {
    protected:
        void DumpAsset(AssetDumpingContext& context, const XAssetInfo<IW4MS::AssetWeapon::Type>& asset) override;
    };
} // namespace weapon
