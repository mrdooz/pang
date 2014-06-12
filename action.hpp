#pragma once
#include "pos.hpp"
#include "types.hpp"

namespace pang
{
  enum class ActionType
  {
    Move,
    MoveTo,
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
    ActionMove(EntityId id) : ActionBase(ActionType::Move) { entityId = id; }
    Vector2f dir;
  };

  struct ActionMoveTo : public ActionBase
  {
    ActionMoveTo(EntityId id) : ActionBase(ActionType::MoveTo) { entityId = id; }
    // note, only the hi part of the position is used for the move
    Vector2f from, to;
    ptime startTime, endTime;
  };

  struct ActionBullet : public ActionBase
  {
    ActionBullet(EntityId id) : ActionBase(ActionType::Bullet) { entityId = id; }
    Vector2f pos;
    Vector2f dir;
  };
}
