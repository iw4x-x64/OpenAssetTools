#pragma once

#include "IPostProcessor.h"

/**
 * \brief Computes the alignment used when allocating a structure, for games where that differs from
 * the alignment implied by the structure's own layout.
 *
 * See IDataRepository::GetAllocAlignmentWordSize. When the two word sizes agree this does nothing,
 * so every existing game is unaffected.
 */
class CalculateAllocAlignmentPostProcessor final : public IPostProcessor
{
public:
    bool PostProcess(IDataRepository* repository) override;
};
