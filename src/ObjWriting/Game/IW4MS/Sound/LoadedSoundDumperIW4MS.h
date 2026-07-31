#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/IW4MS/IW4MS.h"

namespace sound
{
    class LoadedSoundDumperIW4MS final : public AbstractAssetDumper<IW4MS::AssetLoadedSound>
    {
    protected:
        void DumpAsset(AssetDumpingContext& context, const XAssetInfo<IW4MS::AssetLoadedSound::Type>& asset) override;
    };
} // namespace sound
