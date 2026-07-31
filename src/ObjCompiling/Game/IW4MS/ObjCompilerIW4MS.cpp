#include "ObjCompilerIW4MS.h"

#include "Game/IW4MS/Font/FontCompilerIW4MS.h"
#include "Game/IW4MS/IW4MS.h"
#include "Game/IW4MS/Techset/TechniqueCompilerIW4MS.h"
#include "Game/IW4MS/Techset/TechsetCompilerIW4MS.h"
#include "Game/IW4MS/Techset/VertexDeclCompilerIW4MS.h"
#include "Image/ImageIwdPostProcessor.h"

#include <memory>

using namespace IW4MS;

namespace
{
    void ConfigureCompilers(AssetCreatorCollection& collection, Zone& zone, ISearchPath& searchPath)
    {
        auto& memory = zone.Memory();

        collection.AddAssetCreator(techset::CreateVertexDeclCompilerIW4MS(memory));
        collection.AddAssetCreator(techset::CreateTechsetCompilerIW4MS(memory, searchPath));
        collection.AddAssetCreator(font::CreateCompilerIW4MS(memory, searchPath));

        collection.AddSubAssetCreator(techset::CreateTechniqueCompilerIW4MS(memory, zone, searchPath));
    }

    void ConfigurePostProcessors(AssetCreatorCollection& collection,
                                 const ZoneDefinitionContext& zoneDefinition,
                                 ISearchPath& searchPath,
                                 ZoneAssetCreationStateContainer& zoneStates,
                                 IOutputPath& outDir)
    {
        if (image::IwdPostProcessor<AssetImage>::AppliesToZoneDefinition(zoneDefinition))
            collection.AddAssetPostProcessor(std::make_unique<image::IwdPostProcessor<AssetImage>>(zoneDefinition, searchPath, zoneStates, outDir));
    }
} // namespace

namespace IW4MS
{
    void ObjCompiler::ConfigureCreatorCollection(AssetCreatorCollection& collection,
                                                 Zone& zone,
                                                 const ZoneDefinitionContext& zoneDefinition,
                                                 ISearchPath& searchPath,
                                                 [[maybe_unused]] IGdtQueryable& gdt,
                                                 ZoneAssetCreationStateContainer& zoneStates,
                                                 IOutputPath& outDir,
                                                 [[maybe_unused]] IOutputPath& cacheDir) const
    {
        ConfigureCompilers(collection, zone, searchPath);
        ConfigurePostProcessors(collection, zoneDefinition, searchPath, zoneStates, outDir);
    }
} // namespace IW4MS
