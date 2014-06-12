#pragma once
#include "pos.hpp"
#include "types.hpp"

namespace pang
{
  struct Entity
  {
    Entity();
    Vector2f Dir() const;
    Vector2f _pos;
    ptime _lastAction;
    // 0 points straight up, and rotates clockwise. In SFML, (0,-1) points straight up
    float _rot;
    float _vel;
    EntityId _id;
    // fov is symmetric along the direction vector
    float _fov;
    float _viewDistance;

    vector<EntityId> _visibleEntities;
  };

  struct AiState
  {
    Vector2f _lastKnownPlayerState;
  };
}
