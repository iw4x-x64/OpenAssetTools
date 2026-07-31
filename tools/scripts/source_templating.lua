--- Makes a project use the sources templated for it by the SourceTemplates project.
--
function useSourceTemplating(projectName)
    local createdFiles = {}

    local templates = SourceTemplates:templatesOf(projectName)
    for i = 1, #templates do
        for j = 1, #templates[i].outputs do
            table.insert(createdFiles, templates[i].outputs[j])
        end
    end

    includedirs {
        SourceTemplates:outputFolder(projectName)
    }

    files {
        createdFiles
    }

    SourceTemplates:use()
end
