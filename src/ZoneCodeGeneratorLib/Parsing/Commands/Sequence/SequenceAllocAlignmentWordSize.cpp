#include "SequenceAllocAlignmentWordSize.h"

#include "Parsing/Commands/Matcher/CommandsCommonMatchers.h"
#include "Parsing/Commands/Matcher/CommandsMatcherFactory.h"

namespace
{
    static constexpr auto CAPTURE_WORD_SIZE = 1;
}

SequenceAllocAlignmentWordSize::SequenceAllocAlignmentWordSize()
{
    const CommandsMatcherFactory create(this);

    AddMatchers({
        create.Keyword("allocalignwordsize"),
        create.Integer().Capture(CAPTURE_WORD_SIZE),
        create.Char(';'),
    });
}

void SequenceAllocAlignmentWordSize::ProcessMatch(CommandsParserState* state, SequenceResult<CommandsParserValue>& result) const
{
    const auto& wordSizeToken = result.NextCapture(CAPTURE_WORD_SIZE);

    switch (wordSizeToken.IntegerValue())
    {
    case 32:
        state->SetAllocAlignmentWordSize(WordSize::BITS_32);
        break;

    case 64:
        state->SetAllocAlignmentWordSize(WordSize::BITS_64);
        break;

    default:
        throw ParsingException(wordSizeToken.GetPos(), "Unknown word size");
    }
}
