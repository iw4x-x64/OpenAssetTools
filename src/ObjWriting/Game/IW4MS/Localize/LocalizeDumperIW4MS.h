#pragma once

#include "Dumping/AbstractAssetDumper.h"
#include "Game/IW4MS/IW4MS.h"

namespace localize
{
    class DumperIW4MS final : public AbstractSingleProgressAssetDumper<IW4MS::AssetLocalize>
    {
    public:
        void Dump(AssetDumpingContext& context) override;
    };
} // namespace localize
