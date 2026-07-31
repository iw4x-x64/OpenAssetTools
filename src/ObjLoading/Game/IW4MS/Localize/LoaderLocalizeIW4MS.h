#pragma once

#include "Asset/IAssetCreator.h"
#include "Game/IW4MS/IW4MS.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"
#include "Zone/Zone.h"

#include <memory>

namespace localize
{
    std::unique_ptr<AssetCreator<IW4MS::AssetLocalize>> CreateLoaderIW4MS(MemoryManager& memory, ISearchPath& searchPath, Zone& zone);
} // namespace localize
