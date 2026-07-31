#pragma once

#include "Game/IGame.h"
#include "Zone/Zone.h"

#include <memory>

namespace retarget
{
    /**
     * \brief Rewrites a loaded zone as another target's zone, without
     * going through source.
     *
     * This exists for one case: the same game shipped for two word
     * sizes. IW4 and IW4MS are the same source recompiled, so on an x64
     * host their asset structs are the same objects in memory. That is,
     * 34 of the 35 root types are identical in size and alignment, and
     * clipMap_t differs only in trailing padding, every field being at
     * the same offset. Nothing has to be converted field by field and
     * the assets are handed to the other target's writer as they are.
     *
     * \param source A zone already loaded for \p source.m_game_id. It
     *        owns the asset memory and must outlive the returned zone,
     *        which points into it. \param targetGame The game to write
     *        as. \return The retargeted zone, or nullptr if the pair is
     *        not supported, which is reported.
     */
    [[nodiscard]] std::unique_ptr<Zone> Retarget(const Zone& source, GameId targetGame);
} // namespace retarget
