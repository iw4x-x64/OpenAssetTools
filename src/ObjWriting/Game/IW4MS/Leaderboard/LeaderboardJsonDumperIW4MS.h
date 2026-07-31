#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/IW4MS/IW4MS.h"

namespace leaderboard
{
    class JsonDumperIW4MS final : public AbstractAssetDumper<IW4MS::AssetLeaderboard>
    {
    protected:
        void DumpAsset(AssetDumpingContext& context, const XAssetInfo<IW4MS::AssetLeaderboard::Type>& asset) override;
    };
} // namespace leaderboard
