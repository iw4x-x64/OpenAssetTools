#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/IW4MS/IW4MS.h"

namespace sound_curve
{
    class DumperIW4MS final : public AbstractAssetDumper<IW4MS::AssetSoundCurve>
    {
    protected:
        void DumpAsset(AssetDumpingContext& context, const XAssetInfo<IW4MS::AssetSoundCurve::Type>& asset) override;
    };
} // namespace sound_curve
