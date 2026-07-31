SourceTemplates = {}

SourceTemplates.Consumers = {
    "Linking",
    "ObjCommon",
    "ObjCompiling",
    "ObjLoading",
    "ObjWriting",
    "Unlinking"
}

function SourceTemplates:outputFolder(consumerName)
    return "%{wks.location}/src/" .. consumerName
end

function SourceTemplates:templatesOf(consumerName)
    local consumerFolder = path.join(ProjectFolder(), consumerName)
    local templateFiles = os.matchfiles(path.join(consumerFolder, "**.template"))
    local result = {}

    for i = 1, #templateFiles do
        local templateFile = templateFiles[i]
        local relativeTemplatePath = path.getrelative(consumerFolder, templateFile)
        local relativeResultPath = path.replaceextension(relativeTemplatePath, "")
        local resultExtension = path.getextension(relativeResultPath)

        local data = io.readfile(templateFile)
        local gameOptionsStart, gameOptionsEnd = string.find(data, "#options%s+GAME%s*%(")

        if gameOptionsStart == nil then
            error("Source template " .. relativeTemplatePath .. " must define an option called GAME")
        end

        local gameOptionsArgsStart, gameOptionsArgsEnd = string.find(data, "[%a%d%s,]+%)", gameOptionsEnd + 1)

        if gameOptionsArgsStart ~= gameOptionsEnd + 1 then
            error("Source template " .. relativeTemplatePath .. " must define an option called GAME")
        end

        local games = string.explode(string.sub(data, gameOptionsArgsStart, gameOptionsArgsEnd - 1), ",%s*")
        local outputs = {}

        for j = 1, #games do
            local gameName = games[j]
            local outputFileName = path.replaceextension(path.replaceextension(relativeResultPath, "") .. gameName, resultExtension)

            table.insert(outputs, self:outputFolder(consumerName) .. "/Game/" .. gameName .. "/" .. outputFileName)
        end

        table.insert(result, {
            template = templateFile,
            relativePath = relativeTemplatePath,
            logFile = path.replaceextension(relativeTemplatePath, ".%{cfg.platform}.log"),
            outputs = outputs
        })
    end

    return result
end

function SourceTemplates:include(includes)
    if includes:handle(self:name()) then
        for i = 1, #self.Consumers do
            includedirs {
                self:outputFolder(self.Consumers[i])
            }
        end
    end
end

function SourceTemplates:link(links)

end

function SourceTemplates:use()
    dependson(self:name())
end

function SourceTemplates:name()
    return "SourceTemplates"
end

function SourceTemplates:project()
    local includes = Includes:create()

    project(self:name())
        targetdir(TargetDirectoryLib)
        location "%{wks.location}/src/%{prj.name}"
        kind "Utility"

        RawTemplater:use()

        for i = 1, #self.Consumers do
            local consumerName = self.Consumers[i]
            local templates = self:templatesOf(consumerName)

            for j = 1, #templates do
                local template = templates[j]

                files {
                    template.template
                }

                filter("files:" .. template.template)
                    buildmessage("Templating " .. consumerName .. "/" .. template.relativePath)
                    buildinputs {
                        TargetDirectoryBuildTools .. "/" .. ExecutableByOs('RawTemplater')
                    }
                    buildcommands {
                        '"' .. TargetDirectoryBuildTools .. '/' .. ExecutableByOs('RawTemplater') .. '"'
                        .. ' -o "' .. self:outputFolder(consumerName) .. '/"'
                        .. ' --build-log "' .. self:outputFolder(consumerName) .. '/' .. template.logFile .. '"'
                        .. " %{file.abspath}"
                    }
                    buildoutputs {
                        template.outputs
                    }
                filter {}
            end
        end

        vpaths {
            ["*"] = {
                ProjectFolder()
            }
        }
end
