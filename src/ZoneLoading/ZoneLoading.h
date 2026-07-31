#pragma once

#include "Utils/ProgressCallback.h"
#include "Zone/Zone.h"

#include <expected>
#include <string>

class ZoneLoading
{
public:
    /**
     * \brief Loads a zone from a fastfile.
     * \param path Path of the fastfile.
     * \param progressCallback Optional progress reporting.
     * \param forcedGame When set, the zone is loaded as this game instead of detecting the game from
     *                   the fastfile header. Necessary for targets whose header is ambiguous, most
     *                   notably IW4MS, whose retail Microsoft x64 fastfiles are byte-identical to
     *                   retail IW4 x86 fastfiles in the header.
     */
    static std::expected<std::unique_ptr<Zone>, std::string> LoadZone(const std::string& path,
                                                                      std::optional<std::unique_ptr<ProgressCallback>> progressCallback,
                                                                      std::optional<GameId> forcedGame = std::nullopt);
};
