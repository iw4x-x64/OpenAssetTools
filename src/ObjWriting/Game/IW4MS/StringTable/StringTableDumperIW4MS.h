#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/IW4MS/IW4MS.h"

namespace string_table
{
    class DumperIW4MS final : public AbstractAssetDumper<IW4MS::AssetStringTable>
    {
    protected:
        void DumpAsset(AssetDumpingContext& context, const XAssetInfo<IW4MS::AssetStringTable::Type>& asset) override;
    };
} // namespace string_table
