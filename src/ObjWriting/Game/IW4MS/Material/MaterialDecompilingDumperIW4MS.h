#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/IW4MS/IW4MS.h"

namespace material
{
    class DecompilingGdtDumperIW4MS final : public AbstractAssetDumper<IW4MS::AssetMaterial>
    {
    protected:
        void DumpAsset(AssetDumpingContext& context, const XAssetInfo<IW4MS::AssetMaterial::Type>& asset) override;
    };
} // namespace material
