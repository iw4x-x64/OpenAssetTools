#pragma once

#include "Base64.h"
#include "Dumping/ReferencedAssetName.h"

#include <cstddef>
#include <nlohmann/json.hpp>

namespace world
{
    [[nodiscard]] inline nlohmann::json Vec2(const float (&v)[2])
    {
        return nlohmann::json::array({v[0], v[1]});
    }

    [[nodiscard]] inline nlohmann::json Vec3(const float (&v)[3])
    {
        return nlohmann::json::array({v[0], v[1], v[2]});
    }

    [[nodiscard]] inline nlohmann::json Vec4(const float (&v)[4])
    {
        return nlohmann::json::array({v[0], v[1], v[2], v[3]});
    }

    template<typename ElementType, typename TransformFn>
    [[nodiscard]] nlohmann::json Array(const ElementType* elements, const size_t count, TransformFn&& transform)
    {
        auto result = nlohmann::json::array();

        if (!elements)
            return result;

        for (size_t i = 0; i < count; i++)
            result.emplace_back(transform(elements[i]));

        return result;
    }

    template<typename ElementType> [[nodiscard]] nlohmann::json Array(const ElementType* elements, const size_t count)
    {
        return Array(elements,
                     count,
                     [](const ElementType& element)
                     {
                         return element;
                     });
    }

    [[nodiscard]] inline nlohmann::json Base64(const void* data, const size_t size)
    {
        if (!data || !size)
            return "";

        return base64::EncodeBase64(data, size);
    }

    template<size_t Size> void WriteIfNonZero(nlohmann::json& parent, const char* key, const unsigned char (&bytes)[Size])
    {
        for (const auto byte : bytes)
        {
            if (byte)
            {
                parent[key] = nlohmann::json(bytes);
                return;
            }
        }
    }
} // namespace world
