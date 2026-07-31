#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/IW4MS/IW4MS.h"

namespace structured_data_def
{
    class DumperIW4MS final : public AbstractAssetDumper<IW4MS::AssetStructuredDataDef>
    {
    protected:
        void DumpAsset(AssetDumpingContext& context, const XAssetInfo<IW4MS::AssetStructuredDataDef::Type>& asset) override;
    };
} // namespace structured_data_def
