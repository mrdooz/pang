#pragma once
#include "pos.hpp"

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
    u32 _id;
  };
}
