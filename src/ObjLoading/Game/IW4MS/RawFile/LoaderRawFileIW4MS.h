#pragma once

#include "Asset/IAssetCreator.h"
#include "Game/IW4MS/IW4MS.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

#include <memory>

namespace raw_file
{
    std::unique_ptr<AssetCreator<IW4MS::AssetRawFile>> CreateLoaderIW4MS(MemoryManager& memory, ISearchPath& searchPath);
} // namespace raw_file
