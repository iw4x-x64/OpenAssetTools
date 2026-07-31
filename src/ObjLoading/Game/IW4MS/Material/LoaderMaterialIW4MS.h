#pragma once

#include "Asset/IAssetCreator.h"
#include "Game/IW4MS/IW4MS.h"
#include "Gdt/IGdtQueryable.h"
#include "SearchPath/ISearchPath.h"
#include "Utils/MemoryManager.h"

namespace material
{
    std::unique_ptr<AssetCreator<IW4MS::AssetMaterial>> CreateLoaderIW4MS(MemoryManager& memory, ISearchPath& searchPath);
} // namespace material
