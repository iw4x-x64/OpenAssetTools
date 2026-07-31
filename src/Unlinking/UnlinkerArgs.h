#pragma once

#include "Utils/Arguments/ArgumentParser.h"
#include "Zone/Zone.h"

#include <cstdint>
#include <optional>
#include <regex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class UnlinkerArgs
{
public:
    static constexpr const char* DEFAULT_OUTPUT_FOLDER = "zone_dump/zone_raw/?zone?";

private:
    ArgumentParser m_argument_parser;
    std::regex m_zone_pattern;

    /**
     * \brief Prints a command line usage help text for the Unlinker tool to stdout.
     */
    void PrintUsage() const;
    static void PrintVersion();
    bool SetImageDumpingMode() const;
    bool SetModelDumpingMode() const;

    void AddSpecifiedAssetType(std::string value);
    void ParseCommaSeparatedAssetTypeString(const std::string& input);

public:
    enum class ProcessingTask
    {
        DUMP,
        LIST
    };

    enum class AssetTypeHandling : std::uint8_t
    {
        EXCLUDE,
        INCLUDE
    };

    std::vector<std::string> m_zones_to_load;
    std::vector<std::string> m_zones_to_unlink;
    std::set<std::string> m_user_search_paths;

    ProcessingTask m_task;
    std::string m_output_folder;
    bool m_minimal_zone_def;

    std::vector<std::string> m_specified_asset_types;
    std::unordered_map<std::string, size_t> m_specified_asset_type_map;
    AssetTypeHandling m_asset_type_handling;

    bool m_skip_obj;
    bool m_use_gdt;

    /** When set, zones are loaded as this game instead of detecting it from the fastfile header. */
    std::optional<GameId> m_forced_game;

    /** When set, the zone is written back out as this game rather than having its assets dumped. */
    std::optional<GameId> m_convert_to_game;

    /** When set, every zone stream operation is recorded to this file. */
    std::optional<std::string> m_stream_trace_file;

    UnlinkerArgs();
    bool ParseArgs(int argc, const char** argv, bool& shouldContinue);

    /**
     * \brief Converts the output path specified by command line arguments to a path applies for the specified zone.
     * \param zone The zone to resolve the path input for.
     * \return An output path for the zone based on the user input.
     */
    std::string GetOutputFolderPathForZone(const Zone& zone) const;
};
