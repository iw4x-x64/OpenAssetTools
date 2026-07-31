#pragma once

#include "Asset/IAssetCreator.h"
#include "Game/IW4MS/IW4MS.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

#include <memory>

namespace phys_preset
{
    std::unique_ptr<AssetCreator<IW4MS::AssetPhysPreset>> CreateRawLoaderIW4MS(MemoryManager& memory, ISearchPath& searchPath, Zone& zone);
} // namespace phys_preset
