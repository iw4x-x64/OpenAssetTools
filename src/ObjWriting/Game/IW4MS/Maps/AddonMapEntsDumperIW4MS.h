#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/IW4MS/IW4MS.h"

namespace addon_map_ents
{
    class DumperIW4MS final : public AbstractAssetDumper<IW4MS::AssetAddonMapEnts>
    {
    protected:
        void DumpAsset(AssetDumpingContext& context, const XAssetInfo<IW4MS::AssetAddonMapEnts::Type>& asset) override;
    };
} // namespace addon_map_ents
