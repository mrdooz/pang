#pragma once
#include "pos.hpp"

namespace pang
{
  enum class ActionType
  {
    Move,
  };

  struct ActionBase
  {
    ActionType type;
    u32 playerId;
  };

  struct ActionMove : public ActionBase
  {
    // note, only the hi part of the position is used for the move
    Vector2f from, to;
    ptime startTime, endTime;
  };
}
