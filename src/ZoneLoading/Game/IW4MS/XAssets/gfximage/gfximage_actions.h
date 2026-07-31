#pragma once

#include "Game/IW4MS/IW4MS.h"
#include "Loading/AssetLoadingActions.h"

namespace IW4MS
{
    class Actions_GfxImage final : public AssetLoadingActions
    {
    public:
        explicit Actions_GfxImage(Zone& zone);

        void OnImageLoaded(GfxImage* image) const;
        void LoadImageData(GfxImageLoadDef* loadDef, GfxImage* image) const;
    };
} // namespace IW4MS
