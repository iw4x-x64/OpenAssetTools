#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/IW4MS/IW4MS.h"

namespace phys_preset
{
    class InfoStringDumperIW4MS final : public AbstractAssetDumper<IW4MS::AssetPhysPreset>
    {
    protected:
        void DumpAsset(AssetDumpingContext& context, const XAssetInfo<IW4MS::AssetPhysPreset::Type>& asset) override;
    };
} // namespace phys_preset
