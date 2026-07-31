#pragma once

#include "Asset/IAssetCreator.h"
#include "Game/IW4MS/IW4MS.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

#include <memory>

namespace weapon
{
    std::unique_ptr<AssetCreator<IW4MS::AssetWeapon>> CreateRawLoaderIW4MS(MemoryManager& memory, ISearchPath& searchPath, Zone& zone);
} // namespace weapon
