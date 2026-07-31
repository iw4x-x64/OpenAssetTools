ZoneCode = {}

ZoneCode.Assets = {
    IW3 = {
        "PhysPreset",
        "XAnimParts",
        "XModel",
        "Material",
        "MaterialTechniqueSet",
        "GfxImage",
        "snd_alias_list_t",
        "SndCurve",
        "LoadedSound",
        "clipMap_t",
        "ComWorld",
        "GameWorldSp",
        "GameWorldMp",
        "MapEnts",
        "GfxWorld",
        "GfxLightDef",
        "Font_s",
        "MenuList",
        "menuDef_t",
        "LocalizeEntry",
        "WeaponDef",
        "FxEffectDef",
        "FxImpactTable",
        "RawFile",
        "StringTable"
    },

    IW4 = {
        "PhysPreset",
        "PhysCollmap",
        "XAnimParts",
        "XModel",
        "Material",
        "MaterialPixelShader",
        "MaterialVertexShader",
        "MaterialVertexDeclaration",
        "MaterialTechniqueSet",
        "GfxImage",
        "snd_alias_list_t",
        "SndCurve",
        "LoadedSound",
        "clipMap_t",
        "ComWorld",
        "GameWorldSp",
        "GameWorldMp",
        "MapEnts",
        "FxWorld",
        "GfxWorld",
        "GfxLightDef",
        "Font_s",
        "MenuList",
        "menuDef_t",
        "LocalizeEntry",
        "WeaponCompleteDef",
        "FxEffectDef",
        "FxImpactTable",
        "RawFile",
        "StringTable",
        "LeaderboardDef",
        "StructuredDataDefSet",
        "TracerDef",
        "VehicleDef",
        "AddonMapEnts"
    },

    IW5 = {
        "PhysPreset",
        "PhysCollmap",
        "XAnimParts",
        "XModelSurfs",
        "XModel",
        "Material",
        "MaterialPixelShader",
        "MaterialVertexShader",
        "MaterialVertexDeclaration",
        "MaterialTechniqueSet",
        "GfxImage",
        "snd_alias_list_t",
        "SndCurve",
        "LoadedSound",
        "clipMap_t",
        "ComWorld",
        "GlassWorld",
        "PathData",
        "VehicleTrack",
        "MapEnts",
        "FxWorld",
        "GfxWorld",
        "GfxLightDef",
        "Font_s",
        "MenuList",
        "menuDef_t",
        "LocalizeEntry",
        "WeaponAttachment",
        "WeaponCompleteDef",
        "FxEffectDef",
        "FxImpactTable",
        "SurfaceFxTable",
        "RawFile",
        "ScriptFile",
        "StringTable",
        "LeaderboardDef",
        "StructuredDataDefSet",
        "TracerDef",
        "VehicleDef",
        "AddonMapEnts",
    },

    T4 = {
        "PhysPreset",
        "PhysConstraints",
        "DestructibleDef",
        "XAnimParts",
        "XModel",
        "Material",
        "MaterialTechniqueSet",
        "GfxImage",
        "snd_alias_list_t",
        "SndDriverGlobals",
        "LoadedSound",
        "clipMap_t",
        "ComWorld",
        "GameWorldSp",
        "GameWorldMp",
        "MapEnts",
        "GfxWorld",
        "GfxLightDef",
        "Font_s",
        "MenuList",
        "menuDef_t",
        "LocalizeEntry",
        "WeaponDef",
        "FxEffectDef",
        "FxImpactTable",
        "RawFile",
        "StringTable",
        "PackIndex",
    },

    T5 = {
        "PhysPreset",
        "PhysConstraints",
        "DestructibleDef",
        "XAnimParts",
        "XModel",
        "Material",
        "MaterialTechniqueSet",
        "GfxImage",
        "SndBank",
        "SndPatch",
        "clipMap_t",
        "ComWorld",
        "GameWorldSp",
        "GameWorldMp",
        "MapEnts",
        "GfxWorld",
        "GfxLightDef",
        "Font_s",
        "MenuList",
        "menuDef_t",
        "LocalizeEntry",
        "WeaponVariantDef",
        "SndDriverGlobals",
        "FxEffectDef",
        "FxImpactTable",
        "RawFile",
        "StringTable",
        "PackIndex",
        "XGlobals",
        "ddlRoot_t",
        "Glasses",
        "EmblemSet"
    },

    T6 = {
        "PhysPreset",
        "PhysConstraints",
        "DestructibleDef",
        "XAnimParts",
        "XModel",
        "Material",
        "MaterialTechniqueSet",
        "GfxImage",
        "SndBank",
        "SndPatch",
        "clipMap_t",
        "ComWorld",
        "GameWorldSp",
        "GameWorldMp",
        "MapEnts",
        "GfxWorld",
        "GfxLightDef",
        "Font_s",
        "FontIcon",
        "MenuList",
        "menuDef_t",
        "LocalizeEntry",
        "WeaponVariantDef",
        "WeaponAttachment",
        "WeaponAttachmentUnique",
        "WeaponCamo",
        "SndDriverGlobals",
        "FxEffectDef",
        "FxImpactTable",
        "RawFile",
        "StringTable",
        "LeaderboardDef",
        "XGlobals",
        "ddlRoot_t",
        "Glasses",
        "EmblemSet",
        "ScriptParseTree",
        "KeyValuePairs",
        "VehicleDef",
        "MemoryBlock",
        "AddonMapEnts",
        "TracerDef",
        "SkinnedVertsDef",
        "Qdb",
        "Slug",
        "FootstepTableDef",
        "FootstepFXTableDef",
        "ZBarrierDef"
    }
}

ZoneCode.Assets64 = {
    IW4MS = ZoneCode.Assets.IW4
}

local function collectPerAsset(games, suffix)
    local result = {}

    for game, assets in pairs(games) do
        local gameLower = string.lower(game)

        for i, assetName in ipairs(assets) do
            local assetNameLower = string.lower(assetName)
            table.insert(result, "%{wks.location}/src/ZoneCode/%{cfg.platform}/Game/" .. game .. "/XAssets/" .. assetNameLower .. "/" .. assetNameLower .. "_" .. gameLower .. "_" .. suffix .. ".cpp")

            if suffix ~= "struct_test" then
                table.insert(result, "%{wks.location}/src/ZoneCode/%{cfg.platform}/Game/" .. game .. "/XAssets/" .. assetNameLower .. "/" .. assetNameLower .. "_" .. gameLower .. "_" .. suffix .. ".h")
            end
        end
    end

    return result
end

local function collectPerTemplate(games, prefix)
    local result = {}

    for game, assets in pairs(games) do
        table.insert(result, "%{wks.location}/src/ZoneCode/%{cfg.platform}/Game/" .. game .. "/" .. prefix .. game .. ".h")
    end

    return result
end

local function collect(games, suffix, prefix)
    local result = collectPerAsset(games, suffix)

    if prefix then
        for i, file in ipairs(collectPerTemplate(games, prefix)) do
            table.insert(result, file)
        end
    end

    return result
end

function ZoneCode:allTestFiles()
    return collect(self.Assets, "struct_test")
end

function ZoneCode:allTestFiles64()
    return collect(self.Assets64, "struct_test")
end

function ZoneCode:allMarkFiles()
    return collect(self.Assets, "mark_db", "AssetMarker")
end

function ZoneCode:allMarkFiles64()
    return collect(self.Assets64, "mark_db", "AssetMarker")
end

function ZoneCode:allLoadFiles()
    return collect(self.Assets, "load_db", "AssetLoader")
end

function ZoneCode:allLoadFiles64()
    return collect(self.Assets64, "load_db", "AssetLoader")
end

function ZoneCode:allWriteFiles()
    return collect(self.Assets, "write_db", "AssetWriter")
end

function ZoneCode:allWriteFiles64()
    return collect(self.Assets64, "write_db", "AssetWriter")
end

function ZoneCode:include(includes)
	if includes:handle(self:name()) then
		Common:include(includes)
        includedirs {
            path.join(ProjectFolder(), "ZoneCode"),
            "%{wks.location}/src/ZoneCode/%{cfg.platform}"
        }
    end
end

function ZoneCode:link(links)

end

function ZoneCode:use()
	dependson(self:name())
end

function ZoneCode:name()
    return "ZoneCode"
end

function ZoneCode:project()
	local folder = ProjectFolder()
	local includes = Includes:create()

	project(self:name())
        targetdir(TargetDirectoryLib)
		location "%{wks.location}/src/%{prj.name}"
		kind "Utility"

		files {
			path.join(folder, "ZoneCode/**.gen"),
			path.join(folder, "ZoneCode/**.h"),
			path.join(folder, "ZoneCode/**.txt")
        }

        vpaths {
			["*"] = {
				path.join(folder, "ZoneCode")
			}
		}

        self:include(includes)
        ZoneCodeGenerator:use()

        filter { "files:**IW4MS.gen", "platforms:x86" }
            flags { "ExcludeFromBuild" }

        filter "files:**.gen"
            buildmessage "Generating ZoneCode for game %{file.basename}"
            buildcommands {
                '"' .. TargetDirectoryBuildTools .. '/' .. ExecutableByOs('ZoneCodeGenerator') .. '"'
                    .. ' --no-color'
                    .. ' -h "' .. path.join(path.getabsolute(ProjectFolder()), 'ZoneCode/Game/%{file.basename}/%{file.basename}_ZoneCode.h') .. '"'
                    .. ' -c "' .. path.join(path.getabsolute(ProjectFolder()), 'ZoneCode/Game/%{file.basename}/%{file.basename}_Commands.txt') .. '"'
                    .. ' -o "%{wks.location}/src/ZoneCode/%{cfg.platform}/Game/%{file.basename}"'
                    .. ' --build-log "%{wks.location}/src/ZoneCode/%{cfg.platform}/Game/%{file.basename}.log"'
                    .. ' -g ZoneLoad'
                    .. ' -g ZoneMark'
                    .. ' -g ZoneWrite'
                    .. ' -g AssetStructTests'
            }
            buildinputs {
                path.join(ProjectFolder(), "ZoneCode/Game/%{file.basename}/%{file.basename}_ZoneCode.h"),
                path.join(ProjectFolder(), "ZoneCode/Game/%{file.basename}/%{file.basename}_Commands.txt"),
                path.join(ProjectFolder(), "Common/Game/%{file.basename}/%{file.basename}_Assets.h"),
                TargetDirectoryBuildTools .. "/" .. ExecutableByOs('ZoneCodeGenerator')
            }
            buildoutputs {
                "%{wks.location}/src/ZoneCode/%{cfg.platform}/Game/%{file.basename}.log"
            }
        filter {}
end
