#include "Game/IW4MS/IW4MS.h"

#include <catch2/catch_test_macros.hpp>

namespace test::game::iw4ms::retail_asset_sizes
{
    TEST_CASE("IW4MS: asset sizes match DB_GetXAssetTypeSize", "[iw4ms][retail]")
    {
        using namespace IW4MS;

        REQUIRE(sizeof(PhysPreset) == 56u);                 // ASSET_TYPE_PHYSPRESET
        REQUIRE(sizeof(PhysCollmap) == 88u);                // ASSET_TYPE_PHYSCOLLMAP
        REQUIRE(sizeof(XAnimParts) == 136u);                // ASSET_TYPE_XANIMPARTS
        REQUIRE(sizeof(XModelSurfs) == 48u);                // ASSET_TYPE_XMODEL_SURFS
        REQUIRE(sizeof(XModel) == 408u);                    // ASSET_TYPE_XMODEL
        REQUIRE(sizeof(Material) == 120u);                  // ASSET_TYPE_MATERIAL
        REQUIRE(sizeof(MaterialPixelShader) == 32u);        // ASSET_TYPE_PIXELSHADER
        REQUIRE(sizeof(MaterialVertexShader) == 32u);       // ASSET_TYPE_VERTEXSHADER
        REQUIRE(sizeof(MaterialVertexDeclaration) == 176u); // ASSET_TYPE_VERTEXDECL
        REQUIRE(sizeof(MaterialTechniqueSet) == 408u);      // ASSET_TYPE_TECHNIQUE_SET
        REQUIRE(sizeof(GfxImage) == 40u);                   // ASSET_TYPE_IMAGE
        REQUIRE(sizeof(snd_alias_list_t) == 24u);           // ASSET_TYPE_SOUND
        REQUIRE(sizeof(SndCurve) == 144u);                  // ASSET_TYPE_SOUND_CURVE
        REQUIRE(sizeof(LoadedSound) == 64u);                // ASSET_TYPE_LOADED_SOUND
        REQUIRE(sizeof(clipMap_t) == 512u);                 // ASSET_TYPE_CLIPMAP_SP and _MP
        REQUIRE(sizeof(ComWorld) == 24u);                   // ASSET_TYPE_COMWORLD
        REQUIRE(sizeof(GameWorldSp) == 112u);               // ASSET_TYPE_GAMEWORLD_SP
        REQUIRE(sizeof(GameWorldMp) == 16u);                // ASSET_TYPE_GAMEWORLD_MP
        REQUIRE(sizeof(MapEnts) == 88u);                    // ASSET_TYPE_MAP_ENTS
        REQUIRE(sizeof(FxWorld) == 176u);                   // ASSET_TYPE_FXWORLD
        REQUIRE(sizeof(GfxWorld) == 944u);                  // ASSET_TYPE_GFXWORLD
        REQUIRE(sizeof(GfxLightDef) == 32u);                // ASSET_TYPE_LIGHT_DEF
    }

    TEST_CASE("IW4MS: clipMap_t offsets match the retail loader", "[iw4ms][retail]")
    {
        using namespace IW4MS;

        REQUIRE(offsetof(clipMap_t, name) == 0u);
        REQUIRE(offsetof(clipMap_t, planeCount) == 12u);
        REQUIRE(offsetof(clipMap_t, planes) == 16u);
        REQUIRE(offsetof(clipMap_t, numStaticModels) == 24u);
        REQUIRE(offsetof(clipMap_t, staticModelList) == 32u);
        REQUIRE(offsetof(clipMap_t, numMaterials) == 40u);
        REQUIRE(offsetof(clipMap_t, materials) == 48u);
        REQUIRE(offsetof(clipMap_t, numBrushSides) == 56u);
        REQUIRE(offsetof(clipMap_t, brushsides) == 64u);
        REQUIRE(offsetof(clipMap_t, numBrushEdges) == 72u);
        REQUIRE(offsetof(clipMap_t, brushEdges) == 80u);
        REQUIRE(offsetof(clipMap_t, numNodes) == 88u);
        REQUIRE(offsetof(clipMap_t, nodes) == 96u);
        REQUIRE(offsetof(clipMap_t, numLeafs) == 104u);
        REQUIRE(offsetof(clipMap_t, leafs) == 112u);
        REQUIRE(offsetof(clipMap_t, leafbrushNodesCount) == 120u);
        REQUIRE(offsetof(clipMap_t, leafbrushNodes) == 128u);
        REQUIRE(offsetof(clipMap_t, numLeafBrushes) == 136u);
        REQUIRE(offsetof(clipMap_t, leafbrushes) == 144u);
        REQUIRE(offsetof(clipMap_t, numLeafSurfaces) == 152u);
        REQUIRE(offsetof(clipMap_t, leafsurfaces) == 160u);
        REQUIRE(offsetof(clipMap_t, vertCount) == 168u);
        REQUIRE(offsetof(clipMap_t, verts) == 176u);
        REQUIRE(offsetof(clipMap_t, triCount) == 184u);
        REQUIRE(offsetof(clipMap_t, triIndices) == 192u);
        REQUIRE(offsetof(clipMap_t, triEdgeIsWalkable) == 200u);
        REQUIRE(offsetof(clipMap_t, borderCount) == 208u);
        REQUIRE(offsetof(clipMap_t, borders) == 216u);
        REQUIRE(offsetof(clipMap_t, partitionCount) == 224u);
        REQUIRE(offsetof(clipMap_t, partitions) == 232u);
        REQUIRE(offsetof(clipMap_t, aabbTreeCount) == 240u);
        REQUIRE(offsetof(clipMap_t, aabbTrees) == 248u);
        REQUIRE(offsetof(clipMap_t, numSubModels) == 256u);
        REQUIRE(offsetof(clipMap_t, cmodels) == 264u);
        REQUIRE(offsetof(clipMap_t, numBrushes) == 272u);
        REQUIRE(offsetof(clipMap_t, brushes) == 280u);
        REQUIRE(offsetof(clipMap_t, brushBounds) == 288u);
        REQUIRE(offsetof(clipMap_t, brushContents) == 296u);
        REQUIRE(offsetof(clipMap_t, mapEnts) == 304u);
        REQUIRE(offsetof(clipMap_t, smodelNodeCount) == 312u);
        REQUIRE(offsetof(clipMap_t, smodelNodes) == 320u);
        REQUIRE(offsetof(clipMap_t, dynEntCount) == 328u);
        REQUIRE(offsetof(clipMap_t, dynEntDefList) == 336u);
        REQUIRE(offsetof(clipMap_t, dynEntPoseList) == 352u);
        REQUIRE(offsetof(clipMap_t, dynEntClientList) == 368u);
        REQUIRE(offsetof(clipMap_t, dynEntCollList) == 384u);
    }

    TEST_CASE("IW4MS: zone root layout matches a retail fastfile", "[iw4ms][retail]")
    {
        using namespace IW4MS;

        REQUIRE(sizeof(ScriptStringList) == 16u);
        REQUIRE(offsetof(ScriptStringList, count) == 0u);
        REQUIRE(offsetof(ScriptStringList, strings) == 8u);

        REQUIRE(sizeof(XAsset) == 16u);
        REQUIRE(offsetof(XAsset, type) == 0u);
        REQUIRE(offsetof(XAsset, header) == 8u);

        REQUIRE(sizeof(XAssetList) == 32u);
        REQUIRE(offsetof(XAssetList, stringList) == 0u);
        REQUIRE(offsetof(XAssetList, assetCount) == 16u);
        REQUIRE(offsetof(XAssetList, assets) == 24u);
    }
} // namespace test::game::iw4ms::retail_asset_sizes
