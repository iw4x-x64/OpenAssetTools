#include "ZoneLoading.h"

#include "Loading/IZoneLoaderFactory.h"
#include "Loading/ZoneLoader.h"

#include <filesystem>
#include <format>
#include <fstream>
#include <string>

using namespace std::string_literals;
namespace fs = std::filesystem;

std::expected<std::unique_ptr<Zone>, std::string> ZoneLoading::LoadZone(const std::string& path,
                                                                        std::optional<std::unique_ptr<ProgressCallback>> progressCallback,
                                                                        const std::optional<GameId> forcedGame)
{
    auto zoneName = fs::path(path).filename().replace_extension().string();
    std::ifstream file(path, std::fstream::in | std::fstream::binary);

    if (!file.is_open())
        return std::unexpected(std::format("Could not open file '{}'.", path));

    ZoneHeader header{};
    file.read(reinterpret_cast<char*>(&header), sizeof(header));
    if (file.gcount() != sizeof(header))
        return std::unexpected(std::format("Failed to read zone header from file '{}'.", path));

    std::unique_ptr<ZoneLoader> zoneLoader;
    if (forcedGame)
    {
        // Header inspection is skipped entirely: the caller has told us which game this is. Some
        // targets cannot be identified from the header at all, so their factories never claim a
        // zone during detection and are only reachable this way.
        const auto* factory = IZoneLoaderFactory::GetZoneLoaderFactoryForGame(*forcedGame);
        zoneLoader = factory->CreateLoaderForHeader(header, zoneName, std::move(progressCallback));

        if (!zoneLoader)
            return std::unexpected(std::format("Could not load zone '{}' as {}: magic '{}', version {} (file '{}').",
                                               zoneName,
                                               GameId_Names[static_cast<unsigned>(*forcedGame)],
                                               std::string(reinterpret_cast<const char*>(header.m_magic), sizeof(header.m_magic)),
                                               header.m_version,
                                               path));
    }
    else
    {
        for (auto game = 0u; game < static_cast<unsigned>(GameId::COUNT); game++)
        {
            const auto* factory = IZoneLoaderFactory::GetZoneLoaderFactoryForGame(static_cast<GameId>(game));
            if (factory->InspectZoneHeader(header))
            {
                zoneLoader = factory->CreateLoaderForHeader(header, zoneName, std::move(progressCallback));
                break;
            }
        }

        if (!zoneLoader)
            return std::unexpected(std::format("Could not create factory for zone '{}'.", zoneName));
    }

    auto loadedZone = zoneLoader->LoadZone(file);

    file.close();

    if (!loadedZone)
        return std::unexpected("Loading zone failed."s);

    return std::move(loadedZone);
}
