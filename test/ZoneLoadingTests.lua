ZoneLoadingTests = {}

function ZoneLoadingTests:include(includes)
    if includes:handle(self:name()) then
		includedirs {
			path.join(TestFolder(), "ZoneLoadingTests")
		}
	end
end

function ZoneLoadingTests:link(links)

end

function ZoneLoadingTests:use()

end

function ZoneLoadingTests:name()
    return "ZoneLoadingTests"
end

function ZoneLoadingTests:project()
	local folder = TestFolder()
	local includes = Includes:create()
	local links = Links:create()

	project(self:name())
        targetdir(TargetDirectoryTest)
		location "%{wks.location}/test/%{prj.name}"
		kind "ConsoleApp"
		language "C++"

		files {
			path.join(folder, "ZoneLoadingTests/**.h"),
			path.join(folder, "ZoneLoadingTests/**.cpp")
		}

        vpaths {
			["*"] = {
				path.join(folder, "ZoneLoadingTests")
			}
		}

		self:include(includes)
		Catch2Common:include(includes)
		ObjCommonTestUtils:include(includes)
		ZoneLoading:include(includes)
		catch2:include(includes)

		links:linkto(ObjCommonTestUtils)
		links:linkto(ZoneLoading)
		links:linkto(catch2)
		links:linkto(Catch2Common)
		links:linkall()
end
