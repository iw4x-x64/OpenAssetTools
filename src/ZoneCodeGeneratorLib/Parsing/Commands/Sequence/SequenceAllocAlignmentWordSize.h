#pragma once

#include "Parsing/Commands/Impl/CommandsParser.h"

class SequenceAllocAlignmentWordSize final : public CommandsParser::sequence_t
{
public:
    SequenceAllocAlignmentWordSize();

protected:
    void ProcessMatch(CommandsParserState* state, SequenceResult<CommandsParserValue>& result) const override;
};
