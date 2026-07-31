#pragma once

namespace dumping
{
    [[nodiscard]] inline const char* ReferencedAssetName(const char* name)
    {
        if (!name)
            return "";

        if (name[0] == ',')
            return &name[1];

        return name;
    }
} // namespace dumping
