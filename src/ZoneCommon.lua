ZoneCommon = {}

function ZoneCommon:include(includes)
	if includes:handle(self:name()) then
		includedirs {
			path.join(ProjectFolder(), "ZoneCommon")
		}
		Utils:include(includes)
		XMemCompress:include(includes)
		Common:include(includes)
		ObjCommon:include(includes)
		Parser:include(includes)
		Cryptography:include(includes)
		ZoneCode:include(includes)
		ZoneCode:use()
	end
end

function ZoneCommon:link(links)
	links:add(self:name())
	links:linkto(Common)
	links:linkto(Cryptography)
	links:linkto(ObjCommon)
	links:linkto(Parser)
	links:linkto(Utils)
	links:linkto(XMemCompress)
	ZoneCode:use()
end

function ZoneCommon:use()
	
end

function ZoneCommon:name()
	return "ZoneCommon"
end

function ZoneCommon:project()
	local folder = ProjectFolder()
	local includes = Includes:create()

	project(self:name())
        targetdir(TargetDirectoryLib)
		location "%{wks.location}/src/%{prj.name}"
		kind "StaticLib"
		language "C++"
		
		files {
			path.join(folder, "ZoneCommon/**.h"), 
			path.join(folder, "ZoneCommon/**.cpp"),
			ZoneCode:allMarkFiles()
		}

		-- IW4MS serializes 64 bit pointers, so its generated code is x64 only. See ZoneCode.Assets64.
		filter "platforms:x64"
			files { ZoneCode:allMarkFiles64() }
		filter {}
		
        vpaths {
			["*"] = {
				path.join(folder, "ZoneCommon"),
				path.join(BuildFolder(), "src/ZoneCode")
			}
		}
		
        self:include(includes)
		ZoneCode:include(includes)

		ZoneCode:use()
end
