#pragma once

#include "Asset/IAssetCreator.h"
#include "Game/IW4MS/IW4MS.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

#include <memory>

namespace structured_data_def
{
    std::unique_ptr<AssetCreator<IW4MS::AssetStructuredDataDef>> CreateLoaderIW4MS(MemoryManager& memory, ISearchPath& searchPath);
} // namespace structured_data_def
