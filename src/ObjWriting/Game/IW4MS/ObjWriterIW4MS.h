#pragma once

#include "ObjWriter.h"

namespace IW4MS
{
    class ObjWriter final : public IObjWriter
    {
    protected:
        void RegisterAssetDumpers(AssetDumpingContext& context) override;
    };
} // namespace IW4MS
