#pragma once

#include "Asset/IAssetCreator.h"
#include "Game/IW4MS/IW4MS.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

#include <memory>

namespace light_def
{
    std::unique_ptr<AssetCreator<IW4MS::AssetLightDef>> CreateLoaderIW4MS(MemoryManager& memory, ISearchPath& searchPath);
} // namespace light_def
