#pragma once
#include "pos.hpp"
#include "types.hpp"

namespace pang
{
  struct Entity
  {
    Entity();
    Vector2f _pos;
    ptime _lastAction;
    // 0 points straight up
    float _rot;
    float _vel;
    EntityId _id;
  };

  struct AiState
  {
    Vector2f _lastKnownPlayerState;
    float _fov;
  };
}
