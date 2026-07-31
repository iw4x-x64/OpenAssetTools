#include "UnlinkerArgs.h"

#include "GitVersion.h"
#include "ObjLoading.h"
#include "ObjWriting.h"
#include "Utils/Arguments/UsageInformation.h"
#include "Utils/FileUtils.h"
#include "Utils/Logging/Log.h"
#include "Utils/StringUtils.h"

#include <format>
#include <iostream>
#include <regex>
#include <type_traits>

// clang-format off
const CommandLineOption* const OPTION_HELP =
    CommandLineOption::Builder::Create()
    .WithShortName("?")
    .WithLongName("help")
    .WithDescription("Displays usage information.")
    .Build();

const CommandLineOption* const OPTION_VERSION =
    CommandLineOption::Builder::Create()
    .WithLongName("version")
    .WithDescription("Prints the application version.")
    .Build();

const CommandLineOption* const OPTION_VERBOSE =
    CommandLineOption::Builder::Create()
    .WithShortName("v")
    .WithLongName("verbose")
    .WithDescription("Outputs a lot more and more detailed messages.")
    .Build();

const CommandLineOption* const OPTION_NO_COLOR =
    CommandLineOption::Builder::Create()
    .WithLongName("no-color")
    .WithDescription("Disables colored terminal output.")
    .Build();

const CommandLineOption* const OPTION_MINIMAL_ZONE_FILE =
    CommandLineOption::Builder::Create()
    .WithShortName("min")
    .WithLongName("minimal-zone")
    .WithDescription("Minimizes the size of the zone file output by only including assets that are not a dependency of another asset.")
    .Build();

const CommandLineOption* const OPTION_LOAD = 
    CommandLineOption::Builder::Create()
    .WithShortName("l")
    .WithLongName("load")
    .WithDescription("Loads an existing zone before trying to unlink any zone.")
    .WithParameter("zonePath")
    .Reusable()
    .Build();

const CommandLineOption* const OPTION_LIST =
    CommandLineOption::Builder::Create()
    .WithLongName("list")
    .WithDescription("Lists the contents of a zone instead of writing them to the disk.")
    .Build();

const CommandLineOption* const OPTION_OUTPUT_FOLDER =
    CommandLineOption::Builder::Create()
    .WithShortName("o")
    .WithLongName("output-folder")
    .WithDescription(std::format("Specifies the output folder containing the contents of the unlinked zones. Defaults to \"{}\"", UnlinkerArgs::DEFAULT_OUTPUT_FOLDER))
    .WithParameter("outputFolderPath")
    .Build();

const CommandLineOption* const OPTION_SEARCH_PATH = 
    CommandLineOption::Builder::Create()
    .WithLongName("search-path")
    .WithDescription("Specifies a semi-colon separated list of paths to search for additional game files.")
    .WithParameter("searchPathString")
    .Build();

const CommandLineOption* const OPTION_IMAGE_FORMAT = 
    CommandLineOption::Builder::Create()
    .WithLongName("image-format")
    .WithDescription("Specifies the format of dumped image files. Valid values are: DDS, IWI")
    .WithParameter("imageFormatValue")
    .Build();

const CommandLineOption* const OPTION_MODEL_FORMAT = 
    CommandLineOption::Builder::Create()
    .WithLongName("model-format")
    .WithDescription("Specifies the format of dumped model files. Valid values are: XMODEL_EXPORT, XMODEL_BIN, OBJ, GLTF, GLB")
    .WithParameter("modelFormatValue")
    .Build();

const CommandLineOption* const OPTION_GAME =
    CommandLineOption::Builder::Create()
    .WithLongName("game")
    .WithDescription("Loads zones as the specified game instead of detecting the game from the fastfile header. "
                     "Required for IW4MS, whose retail Microsoft x64 fastfiles have the same header as retail IW4 x86 ones. "
                     "Valid values: IW3, IW4, IW4MS, IW5, T4, T5, T6.")
    .WithParameter("gameName")
    .Build();

const CommandLineOption* const OPTION_CONVERT_TO =
    CommandLineOption::Builder::Create()
    .WithLongName("convert-to")
    .WithDescription("Writes the loaded zone back out as the specified game instead of dumping its assets. "
                     "Only IW4 to IW4MS is supported: they are the same game built for two word sizes, so the "
                     "assets need no conversion. The result goes to the output folder as <zone>.ff.")
    .WithParameter("gameName")
    .Build();

const CommandLineOption* const OPTION_TRACE_STREAM =
    CommandLineOption::Builder::Create()
    .WithLongName("trace-stream")
    .WithDescription("Writes a machine readable record of every zone stream operation to the specified file. "
                     "Two runs of the same zone produce identical traces, so diffing them locates the first divergent read.")
    .WithParameter("traceFile")
    .Build();

const CommandLineOption* const OPTION_SKIP_OBJ =
    CommandLineOption::Builder::Create()
    .WithLongName("skip-obj")
    .WithDescription("Skips loading raw obj data.")
    .Build();

const CommandLineOption* const OPTION_GDT =
    CommandLineOption::Builder::Create()
    .WithLongName("gdt")
    .WithDescription("Dumps assets in a GDT whenever possible.")
    .Build();

const CommandLineOption* const OPTION_EXCLUDE_ASSETS = 
    CommandLineOption::Builder::Create()
    .WithLongName("exclude-assets")
    .WithDescription("Specify all asset types that should be excluded.")
    .WithParameter("assetTypeList")
    .Reusable()
    .Build();

const CommandLineOption* const OPTION_INCLUDE_ASSETS = 
    CommandLineOption::Builder::Create()
    .WithLongName("include-assets")
    .WithDescription("Specify all asset types that should be included.")
    .WithParameter("assetTypeList")
    .Reusable()
    .Build();

const CommandLineOption* const OPTION_LEGACY_MENUS =
    CommandLineOption::Builder::Create()
    .WithLongName("legacy-menus")
    .WithDescription("Dumps menus with a compatibility mode to work with applications not compatible with the newer dumping mode.")
    .Build();

// clang-format on

const CommandLineOption* const COMMAND_LINE_OPTIONS[]{
    OPTION_HELP,
    OPTION_VERSION,
    OPTION_VERBOSE,
    OPTION_NO_COLOR,
    OPTION_MINIMAL_ZONE_FILE,
    OPTION_LOAD,
    OPTION_LIST,
    OPTION_OUTPUT_FOLDER,
    OPTION_SEARCH_PATH,
    OPTION_IMAGE_FORMAT,
    OPTION_MODEL_FORMAT,
    OPTION_GAME,
    OPTION_CONVERT_TO,
    OPTION_TRACE_STREAM,
    OPTION_SKIP_OBJ,
    OPTION_GDT,
    OPTION_EXCLUDE_ASSETS,
    OPTION_INCLUDE_ASSETS,
    OPTION_LEGACY_MENUS,
};

UnlinkerArgs::UnlinkerArgs()
    : m_argument_parser(COMMAND_LINE_OPTIONS, std::extent_v<decltype(COMMAND_LINE_OPTIONS)>),
      m_zone_pattern(R"(\?zone\?)"),
      m_task(ProcessingTask::DUMP),
      m_minimal_zone_def(false),
      m_asset_type_handling(AssetTypeHandling::EXCLUDE),
      m_skip_obj(false),
      m_use_gdt(false)
{
}

void UnlinkerArgs::PrintUsage() const
{
    UsageInformation usage(m_argument_parser.GetExecutableName());

    for (const auto* commandLineOption : COMMAND_LINE_OPTIONS)
    {
        usage.AddCommandLineOption(commandLineOption);
    }

    usage.AddArgument("pathToZone");
    usage.SetVariableArguments(true);

    usage.Print();
}

void UnlinkerArgs::PrintVersion()
{
    con::info("OpenAssetTools Unlinker {}", GIT_VERSION);
}

bool UnlinkerArgs::SetImageDumpingMode() const
{
    auto specifiedValue = m_argument_parser.GetValueForOption(OPTION_IMAGE_FORMAT);
    utils::MakeStringLowerCase(specifiedValue);

    if (specifiedValue == "dds")
    {
        ObjWriting::Configuration.ImageOutputFormat = ImageOutputFormat_e::DDS;
        return true;
    }

    if (specifiedValue == "iwi")
    {
        ObjWriting::Configuration.ImageOutputFormat = ImageOutputFormat_e::IWI;
        return true;
    }

    const std::string originalValue = m_argument_parser.GetValueForOption(OPTION_IMAGE_FORMAT);
    con::error("Illegal value: \"{}\" is not a valid image output format. Use -? to see usage information.", originalValue);
    return false;
}

bool UnlinkerArgs::SetModelDumpingMode() const
{
    auto specifiedValue = m_argument_parser.GetValueForOption(OPTION_MODEL_FORMAT);
    utils::MakeStringLowerCase(specifiedValue);

    if (specifiedValue == "xmodel_export")
    {
        ObjWriting::Configuration.ModelOutputFormat = ModelOutputFormat_e::XMODEL_EXPORT;
        return true;
    }

    if (specifiedValue == "xmodel_bin")
    {
        ObjWriting::Configuration.ModelOutputFormat = ModelOutputFormat_e::XMODEL_BIN;
        return true;
    }

    if (specifiedValue == "obj")
    {
        ObjWriting::Configuration.ModelOutputFormat = ModelOutputFormat_e::OBJ;
        return true;
    }

    if (specifiedValue == "gltf")
    {
        ObjWriting::Configuration.ModelOutputFormat = ModelOutputFormat_e::GLTF;
        return true;
    }

    if (specifiedValue == "glb")
    {
        ObjWriting::Configuration.ModelOutputFormat = ModelOutputFormat_e::GLB;
        return true;
    }

    const std::string originalValue = m_argument_parser.GetValueForOption(OPTION_MODEL_FORMAT);
    con::error("Illegal value: \"{}\" is not a valid model output format. Use -? to see usage information.", originalValue);
    return false;
}

void UnlinkerArgs::AddSpecifiedAssetType(std::string value)
{
    const auto alreadySpecifiedAssetType = m_specified_asset_type_map.find(value);
    if (alreadySpecifiedAssetType == m_specified_asset_type_map.end())
    {
        m_specified_asset_type_map.emplace(std::make_pair(value, m_specified_asset_types.size()));
        m_specified_asset_types.emplace_back(std::move(value));
    }
}

void UnlinkerArgs::ParseCommaSeparatedAssetTypeString(const std::string& input)
{
    auto currentPos = 0uz;
    size_t endPos;

    std::string lowerInput(input);
    utils::MakeStringLowerCase(lowerInput);

    while (currentPos < lowerInput.size() && (endPos = lowerInput.find_first_of(',', currentPos)) != std::string::npos)
    {
        AddSpecifiedAssetType(std::string(lowerInput, currentPos, endPos - currentPos));
        currentPos = endPos + 1;
    }

    if (currentPos < lowerInput.size())
        AddSpecifiedAssetType(std::string(lowerInput, currentPos, lowerInput.size() - currentPos));
}

bool UnlinkerArgs::ParseArgs(const int argc, const char** argv, bool& shouldContinue)
{
    shouldContinue = true;
    if (!m_argument_parser.ParseArguments(argc, argv))
    {
        PrintUsage();
        return false;
    }

    // Check if the user requested help
    if (m_argument_parser.IsOptionSpecified(OPTION_HELP))
    {
        PrintUsage();
        shouldContinue = false;
        return true;
    }

    // Check if the user wants to see the version
    if (m_argument_parser.IsOptionSpecified(OPTION_VERSION))
    {
        PrintVersion();
        shouldContinue = false;
        return true;
    }

    m_zones_to_unlink = m_argument_parser.GetArguments();
    const size_t zoneCount = m_zones_to_unlink.size();
    if (zoneCount < 1)
    {
        // No zones to load specified...
        PrintUsage();
        return false;
    }

    // -v; --verbose
    if (m_argument_parser.IsOptionSpecified(OPTION_VERBOSE))
        con::set_log_level(con::LogLevel::DEBUG);
    else
        con::set_log_level(con::LogLevel::INFO);

    // --no-color
    con::set_use_color(!m_argument_parser.IsOptionSpecified(OPTION_NO_COLOR));

    // -min; --minimal-zone
    m_minimal_zone_def = m_argument_parser.IsOptionSpecified(OPTION_MINIMAL_ZONE_FILE);

    // -l; --load
    if (m_argument_parser.IsOptionSpecified(OPTION_LOAD))
        m_zones_to_load = m_argument_parser.GetParametersForOption(OPTION_LOAD);

    // --list
    if (m_argument_parser.IsOptionSpecified(OPTION_LIST))
        m_task = ProcessingTask::LIST;

    // -o; --output-folder
    if (m_argument_parser.IsOptionSpecified(OPTION_OUTPUT_FOLDER))
        m_output_folder = m_argument_parser.GetValueForOption(OPTION_OUTPUT_FOLDER);
    else
        m_output_folder = DEFAULT_OUTPUT_FOLDER;

    // --search-path
    if (m_argument_parser.IsOptionSpecified(OPTION_SEARCH_PATH))
    {
        if (!utils::ParsePathsString(m_argument_parser.GetValueForOption(OPTION_SEARCH_PATH), m_user_search_paths))
        {
            return false;
        }
    }

    // --image-format
    if (m_argument_parser.IsOptionSpecified(OPTION_IMAGE_FORMAT))
    {
        if (!SetImageDumpingMode())
        {
            return false;
        }
    }

    // --model-format
    if (m_argument_parser.IsOptionSpecified(OPTION_MODEL_FORMAT))
    {
        if (!SetModelDumpingMode())
        {
            return false;
        }
    }

    // --convert-to
    if (m_argument_parser.IsOptionSpecified(OPTION_CONVERT_TO))
    {
        const auto gameName = m_argument_parser.GetValueForOption(OPTION_CONVERT_TO);
        m_convert_to_game = IGame::FindGameIdByName(gameName);

        if (!m_convert_to_game)
        {
            con::error("Unknown game \"{}\" for --convert-to.", gameName);
            return false;
        }
    }

    // --game
    if (m_argument_parser.IsOptionSpecified(OPTION_GAME))
    {
        const auto gameName = m_argument_parser.GetValueForOption(OPTION_GAME);
        m_forced_game = IGame::FindGameIdByName(gameName);

        if (!m_forced_game)
        {
            std::string knownGames;
            for (const auto* knownGame : GameId_Names)
            {
                if (!knownGames.empty())
                    knownGames += ", ";
                knownGames += knownGame;
            }

            con::error("Unknown game \"{}\". Valid values are: {}", gameName, knownGames);
            return false;
        }
    }

    // --trace-stream
    if (m_argument_parser.IsOptionSpecified(OPTION_TRACE_STREAM))
        m_stream_trace_file = m_argument_parser.GetValueForOption(OPTION_TRACE_STREAM);

    // --skip-obj
    m_skip_obj = m_argument_parser.IsOptionSpecified(OPTION_SKIP_OBJ);

    // --gdt
    m_use_gdt = m_argument_parser.IsOptionSpecified(OPTION_GDT);

    // --exclude-assets
    // --include-assets
    if (m_argument_parser.IsOptionSpecified(OPTION_EXCLUDE_ASSETS) && m_argument_parser.IsOptionSpecified(OPTION_INCLUDE_ASSETS))
    {
        con::error("You can only asset types to either exclude or include, not both");
        return false;
    }

    if (m_argument_parser.IsOptionSpecified(OPTION_EXCLUDE_ASSETS))
    {
        m_asset_type_handling = AssetTypeHandling::EXCLUDE;
        for (const auto& exclude : m_argument_parser.GetParametersForOption(OPTION_EXCLUDE_ASSETS))
            ParseCommaSeparatedAssetTypeString(exclude);
    }
    else if (m_argument_parser.IsOptionSpecified(OPTION_INCLUDE_ASSETS))
    {
        m_asset_type_handling = AssetTypeHandling::INCLUDE;
        for (const auto& include : m_argument_parser.GetParametersForOption(OPTION_INCLUDE_ASSETS))
            ParseCommaSeparatedAssetTypeString(include);
    }

    // --legacy-menus
    if (m_argument_parser.IsOptionSpecified(OPTION_LEGACY_MENUS))
        ObjWriting::Configuration.MenuLegacyMode = true;

    return true;
}

std::string UnlinkerArgs::GetOutputFolderPathForZone(const Zone& zone) const
{
    return std::regex_replace(m_output_folder, m_zone_pattern, zone.m_name);
}
