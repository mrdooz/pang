#pragma once
#include "pos.hpp"

namespace pang
{
  enum class ActionType
  {
    Move,
    Bullet
  };

  struct ActionBase
  {
    ActionBase(ActionType type) : type(type) {}
    ActionType type;
    u16 entityId;
  };

  struct ActionMove : public ActionBase
  {
    ActionMove() : ActionBase(ActionType::Move) {}
    // note, only the hi part of the position is used for the move
    Vector2f from, to;
    ptime startTime, endTime;
  };

  struct ActionBullet : public ActionBase
  {
    ActionBullet() : ActionBase(ActionType::Bullet) {}
    Vector2f pos;
    Vector2f dir;
  };
}
