#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/IW4MS/IW4MS.h"

namespace raw_file
{
    class DumperIW4MS final : public AbstractAssetDumper<IW4MS::AssetRawFile>
    {
    protected:
        void DumpAsset(AssetDumpingContext& context, const XAssetInfo<IW4MS::AssetRawFile::Type>& asset) override;
    };
} // namespace raw_file
