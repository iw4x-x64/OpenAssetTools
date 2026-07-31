#pragma once

#include "Game/IW4MS/IW4MS.h"
#include "Loading/AssetLoadingActions.h"

namespace IW4MS
{
    class Actions_LoadedSound final : public AssetLoadingActions
    {
    public:
        explicit Actions_LoadedSound(Zone& zone);

        void SetSoundData(MssSound* sound) const;
    };
} // namespace IW4MS
