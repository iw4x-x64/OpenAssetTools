#include "ZoneLoaderFactoryIW4MS.h"

#ifdef ARCH_x64
#include "ContentLoaderIW4MS.h"
#endif
#include "Game/GameLanguage.h"
#include "Game/IW4MS/IW4MS.h"
#include "Game/IW4MS/ZoneConstantsIW4MS.h"
#include "Loading/Processor/ProcessorAuthedBlocks.h"
#include "Loading/Processor/ProcessorCaptureData.h"
#include "Loading/Processor/ProcessorInflate.h"
#include "Loading/Steps/StepAddProcessor.h"
#include "Loading/Steps/StepAllocXBlocks.h"
#include "Loading/Steps/StepLoadHash.h"
#include "Loading/Steps/StepLoadSignature.h"
#include "Loading/Steps/StepLoadZoneContent.h"
#include "Loading/Steps/StepLoadZoneSizes.h"
#include "Loading/Steps/StepRemoveProcessor.h"
#include "Loading/Steps/StepSkipBytes.h"
#include "Loading/Steps/StepVerifyFileName.h"
#include "Loading/Steps/StepVerifyHash.h"
#include "Loading/Steps/StepVerifyMagic.h"
#include "Loading/Steps/StepVerifySignature.h"
#include "Utils/ClassUtils.h"
#include "Utils/Endianness.h"
#include "Utils/Logging/Log.h"

#include <cstring>
#include <type_traits>

using namespace IW4MS;

namespace
{
    struct HeaderInfo
    {
        bool m_is_official;
        bool m_is_signed;
    };

    std::optional<HeaderInfo> InspectHeader(const ZoneHeader& header)
    {
        if (endianness::FromLittleEndian(header.m_version) != ZoneConstants::ZONE_VERSION_PC)
            return std::nullopt;

        if (!std::memcmp(header.m_magic, ZoneConstants::MAGIC_SIGNED_INFINITY_WARD, std::char_traits<char>::length(ZoneConstants::MAGIC_SIGNED_INFINITY_WARD)))
            return HeaderInfo{.m_is_official = true, .m_is_signed = true};

        if (!std::memcmp(header.m_magic, ZoneConstants::MAGIC_UNSIGNED, std::char_traits<char>::length(ZoneConstants::MAGIC_UNSIGNED)))
            return HeaderInfo{.m_is_official = false, .m_is_signed = false};

        return std::nullopt;
    }

    void SetupBlocks(ZoneLoader& zoneLoader)
    {
#define XBLOCK_DEF(name, type) std::make_unique<XBlock>(STR(name), name, type)

        zoneLoader.AddXBlock(XBLOCK_DEF(XFILE_BLOCK_TEMP, XBlockType::BLOCK_TYPE_TEMP));
        zoneLoader.AddXBlock(XBLOCK_DEF(XFILE_BLOCK_PHYSICAL, XBlockType::BLOCK_TYPE_NORMAL));
        zoneLoader.AddXBlock(XBLOCK_DEF(XFILE_BLOCK_RUNTIME, XBlockType::BLOCK_TYPE_RUNTIME));
        zoneLoader.AddXBlock(XBLOCK_DEF(XFILE_BLOCK_VIRTUAL, XBlockType::BLOCK_TYPE_NORMAL));
        zoneLoader.AddXBlock(XBLOCK_DEF(XFILE_BLOCK_LARGE, XBlockType::BLOCK_TYPE_NORMAL));
        zoneLoader.AddXBlock(XBLOCK_DEF(XFILE_BLOCK_CALLBACK, XBlockType::BLOCK_TYPE_NORMAL));
        zoneLoader.AddXBlock(XBLOCK_DEF(XFILE_BLOCK_VERTEX, XBlockType::BLOCK_TYPE_NORMAL));
        zoneLoader.AddXBlock(XBLOCK_DEF(XFILE_BLOCK_INDEX, XBlockType::BLOCK_TYPE_NORMAL));

#undef XBLOCK_DEF
    }

    std::unique_ptr<cryptography::IPublicKeyAlgorithm> SetupRsa()
    {
        auto rsa = cryptography::CreateRsa(cryptography::HashingAlgorithm::RSA_HASH_SHA256, cryptography::RsaPaddingMode::RSA_PADDING_PSS);

        if (!rsa->SetKey(ZoneConstants::RSA_PUBLIC_KEY_INFINITY_WARD_PC, sizeof(ZoneConstants::RSA_PUBLIC_KEY_INFINITY_WARD_PC)))
        {
            con::error("Invalid public key for signature checking");
            return nullptr;
        }

        return rsa;
    }

    void AddAuthHeaderSteps(const HeaderInfo& headerInfo, ZoneLoader& zoneLoader, const std::string& fileName)
    {
        if (!headerInfo.m_is_signed)
            return;

        auto rsa = SetupRsa();

        zoneLoader.AddLoadingStep(step::CreateStepVerifyMagic(ZoneConstants::MAGIC_AUTH_HEADER));
        zoneLoader.AddLoadingStep(step::CreateStepSkipBytes(4)); // Skip reserved

        auto subHeaderHash = step::CreateStepLoadHash(sizeof(DB_AuthHash::bytes), 1);
        auto* subHeaderHashPtr = subHeaderHash.get();
        zoneLoader.AddLoadingStep(std::move(subHeaderHash));

        auto subHeaderHashSignature = step::CreateStepLoadSignature(sizeof(DB_AuthSignature::bytes));
        auto* subHeaderHashSignaturePtr = subHeaderHashSignature.get();
        zoneLoader.AddLoadingStep(std::move(subHeaderHashSignature));

        zoneLoader.AddLoadingStep(step::CreateStepVerifySignature(std::move(rsa), subHeaderHashSignaturePtr, subHeaderHashPtr));

        auto subHeaderCapture = processor::CreateProcessorCaptureData(sizeof(DB_AuthSubHeader));
        auto* subHeaderCapturePtr = subHeaderCapture.get();
        zoneLoader.AddLoadingStep(step::CreateStepAddProcessor(std::move(subHeaderCapture)));

        zoneLoader.AddLoadingStep(step::CreateStepVerifyFileName(fileName, sizeof(DB_AuthSubHeader::fastfileName)));
        zoneLoader.AddLoadingStep(step::CreateStepSkipBytes(4)); // Skip reserved

        auto masterBlockHashes =
            step::CreateStepLoadHash(sizeof(DB_AuthHash::bytes), static_cast<unsigned>(std::extent_v<decltype(DB_AuthSubHeader::masterBlockHashes)>));
        auto* masterBlockHashesPtr = masterBlockHashes.get();
        zoneLoader.AddLoadingStep(std::move(masterBlockHashes));

        zoneLoader.AddLoadingStep(step::CreateStepVerifyHash(cryptography::CreateSha256(), 0, subHeaderHashPtr, subHeaderCapturePtr));
        zoneLoader.AddLoadingStep(step::CreateStepRemoveProcessor(subHeaderCapturePtr));

        // Skip the rest of the first chunk
        zoneLoader.AddLoadingStep(step::CreateStepSkipBytes(ZoneConstants::AUTHED_CHUNK_SIZE - sizeof(DB_AuthHeader)));

        zoneLoader.AddLoadingStep(step::CreateStepAddProcessor(
            processor::CreateProcessorAuthedBlocks(ZoneConstants::AUTHED_CHUNK_COUNT_PER_GROUP,
                                                   ZoneConstants::AUTHED_CHUNK_SIZE,
                                                   static_cast<unsigned>(std::extent_v<decltype(DB_AuthSubHeader::masterBlockHashes)>),
                                                   cryptography::CreateSha256(),
                                                   masterBlockHashesPtr)));
    }
} // namespace

namespace IW4MS
{
    std::optional<ZoneLoaderInspectionResult> ZoneLoaderFactory::InspectZoneHeader([[maybe_unused]] const ZoneHeader& header) const
    {
        return std::nullopt;
    }

    std::unique_ptr<ZoneLoader> ZoneLoaderFactory::CreateLoaderForHeader(const ZoneHeader& header,
                                                                         const std::string& fileName,
                                                                         std::optional<std::unique_ptr<ProgressCallback>> progressCallback) const
    {
        const auto headerInfo = InspectHeader(header);
        if (!headerInfo)
            return nullptr;

        auto zone = std::make_unique<Zone>(fileName, 0, GameId::IW4MS, GamePlatform::PC);
        auto* zonePtr = zone.get();
        zone->m_language = GameLanguage::LANGUAGE_NONE;

        auto zoneLoader = std::make_unique<ZoneLoader>(std::move(zone));

        SetupBlocks(*zoneLoader);

        // Skip unknown 1 byte field that the game ignores as well
        zoneLoader->AddLoadingStep(step::CreateStepSkipBytes(1));

        // Skip timestamp
        zoneLoader->AddLoadingStep(step::CreateStepSkipBytes(8));

        AddAuthHeaderSteps(*headerInfo, *zoneLoader, fileName);

        zoneLoader->AddLoadingStep(step::CreateStepAddProcessor(processor::CreateProcessorInflate(ZoneConstants::AUTHED_CHUNK_SIZE)));

        // Start of the XFile struct
        zoneLoader->AddLoadingStep(step::CreateStepLoadZoneSizes());
        zoneLoader->AddLoadingStep(step::CreateStepAllocXBlocks());

#ifndef ARCH_x64
        con::error("Loading IW4MS zones needs a 64 bit build: its pointers are 8 bytes wide.");
        return nullptr;
#else
        // Start of the zone content
        zoneLoader->AddLoadingStep(step::CreateStepLoadZoneContent(
            [zonePtr](ZoneInputStream& stream)
            {
                return std::make_unique<ContentLoader>(*zonePtr, stream);
            },
            ZoneConstants::POINTER_BIT_COUNT,
            ZoneConstants::OFFSET_BLOCK_BIT_COUNT,
            ZoneConstants::OFFSET_BIT_COUNT,
            ZoneConstants::INSERT_BLOCK,
            zonePtr->Memory(),
            std::move(progressCallback)));

        return zoneLoader;
#endif // ARCH_x64
    }
} // namespace IW4MS
