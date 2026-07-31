#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/IW4MS/IW4MS.h"

namespace light_def
{
    class DumperIW4MS final : public AbstractAssetDumper<IW4MS::AssetLightDef>
    {
    protected:
        void DumpAsset(AssetDumpingContext& context, const XAssetInfo<IW4MS::AssetLightDef::Type>& asset) override;
    };
} // namespace light_def
