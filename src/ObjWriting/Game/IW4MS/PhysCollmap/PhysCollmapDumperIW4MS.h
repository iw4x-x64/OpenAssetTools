#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/IW4MS/IW4MS.h"

namespace phys_collmap
{
    class DumperIW4MS final : public AbstractAssetDumper<IW4MS::AssetPhysCollMap>
    {
    protected:
        void DumpAsset(AssetDumpingContext& context, const XAssetInfo<IW4MS::AssetPhysCollMap::Type>& asset) override;
    };
} // namespace phys_collmap
