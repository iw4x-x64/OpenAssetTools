#pragma once

#include "Game/IW4MS/IW4MS.h"
#include "Loading/AssetLoadingActions.h"

namespace IW4MS
{
    class Actions_XModel final : public AssetLoadingActions
    {
    public:
        explicit Actions_XModel(Zone& zone);

        void SetModelSurfs(XModelLodInfo* lodInfo, XModelSurfs* modelSurfs) const;
    };
} // namespace IW4MS
