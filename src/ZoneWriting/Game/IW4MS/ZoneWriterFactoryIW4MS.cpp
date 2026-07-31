#include "ZoneWriterFactoryIW4MS.h"

#ifdef ARCH_x64
#include "ContentWriterIW4MS.h"
#endif
#include "Game/IW4MS/GameIW4MS.h"
#include "Game/IW4MS/IW4MS.h"
#include "Game/IW4MS/ZoneConstantsIW4MS.h"
#include "Utils/ClassUtils.h"
#include "Utils/Logging/Log.h"
#include "Writing/Processor/OutputProcessorDeflate.h"
#include "Writing/Steps/StepAddOutputProcessor.h"
#include "Writing/Steps/StepWriteTimestamp.h"
#include "Writing/Steps/StepWriteXBlockSizes.h"
#include "Writing/Steps/StepWriteZero.h"
#include "Writing/Steps/StepWriteZoneContentToFile.h"
#include "Writing/Steps/StepWriteZoneContentToMemory.h"
#include "Writing/Steps/StepWriteZoneHeader.h"
#include "Writing/Steps/StepWriteZoneSizes.h"

#include <cstring>

using namespace IW4MS;

namespace
{
    void SetupBlocks(ZoneWriter& writer)
    {
#define XBLOCK_DEF(name, type) std::make_unique<XBlock>(STR(name), name, type)

        writer.AddXBlock(XBLOCK_DEF(XFILE_BLOCK_TEMP, XBlockType::BLOCK_TYPE_TEMP));
        writer.AddXBlock(XBLOCK_DEF(XFILE_BLOCK_PHYSICAL, XBlockType::BLOCK_TYPE_NORMAL));
        writer.AddXBlock(XBLOCK_DEF(XFILE_BLOCK_RUNTIME, XBlockType::BLOCK_TYPE_RUNTIME));
        writer.AddXBlock(XBLOCK_DEF(XFILE_BLOCK_VIRTUAL, XBlockType::BLOCK_TYPE_NORMAL));
        writer.AddXBlock(XBLOCK_DEF(XFILE_BLOCK_LARGE, XBlockType::BLOCK_TYPE_NORMAL));
        writer.AddXBlock(XBLOCK_DEF(XFILE_BLOCK_CALLBACK, XBlockType::BLOCK_TYPE_NORMAL));
        writer.AddXBlock(XBLOCK_DEF(XFILE_BLOCK_VERTEX, XBlockType::BLOCK_TYPE_NORMAL));
        writer.AddXBlock(XBLOCK_DEF(XFILE_BLOCK_INDEX, XBlockType::BLOCK_TYPE_NORMAL));

#undef XBLOCK_DEF
    }

    ZoneHeader CreateHeader()
    {
        ZoneHeader header{};
        header.m_version = ZoneConstants::ZONE_VERSION_PC;

        memcpy(header.m_magic, ZoneConstants::MAGIC_UNSIGNED, sizeof(ZoneHeader::m_magic));

        return header;
    }
} // namespace

namespace IW4MS
{
#ifndef ARCH_x64
    std::unique_ptr<ZoneWriter> ZoneWriterFactory::CreateWriter([[maybe_unused]] const Zone& zone) const
    {
        con::error("Writing IW4MS zones needs a 64 bit build: its pointers are 8 bytes wide.");
        return nullptr;
    }
#else
    std::unique_ptr<ZoneWriter> ZoneWriterFactory::CreateWriter(const Zone& zone) const
    {
        auto writer = std::make_unique<ZoneWriter>();

        SetupBlocks(*writer);

        auto contentInMemory = std::make_unique<StepWriteZoneContentToMemory>(std::make_unique<ContentWriter>(zone),
                                                                              zone,
                                                                              ZoneConstants::POINTER_BIT_COUNT,
                                                                              ZoneConstants::OFFSET_BLOCK_BIT_COUNT,
                                                                              ZoneConstants::OFFSET_BIT_COUNT,
                                                                              ZoneConstants::INSERT_BLOCK);
        auto* contentInMemoryPtr = contentInMemory.get();
        writer->AddWritingStep(std::move(contentInMemory));

        writer->AddWritingStep(std::make_unique<StepWriteZoneHeader>(CreateHeader()));

        writer->AddWritingStep(std::make_unique<StepWriteZero>(1));
        writer->AddWritingStep(std::make_unique<StepWriteTimestamp>());

        writer->AddWritingStep(std::make_unique<StepAddOutputProcessor>(std::make_unique<OutputProcessorDeflate>()));

        writer->AddWritingStep(std::make_unique<StepWriteZoneSizes>(contentInMemoryPtr));
        writer->AddWritingStep(std::make_unique<StepWriteXBlockSizes>(zone));

        writer->AddWritingStep(std::make_unique<StepWriteZoneContentToFile>(contentInMemoryPtr));

        return writer;
    }
#endif // ARCH_x64
} // namespace IW4MS
