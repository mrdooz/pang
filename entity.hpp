#pragma once
#include "pos.hpp"
#include "types.hpp"

namespace pang
{
  enum class Behavior
  {
    Unknown,
    Seek,
  };

  struct Entity
  {
    Entity();
    Vector2f Dir() const;
    // 0 points straight up, and rotates clockwise. In SFML, (0,-1) points straight up
    Vector2f _acc;
    Vector2f _vel;
    Vector2f _pos;
    ptime _lastAction;
    EntityId _id;
    // fov is symmetric along the direction vector
    float _fov;
    float _viewDistance;

    vector<EntityId> _visibleEntities;
  };

  struct AiState
  {
    AiState() : _behavior(Behavior::Unknown) {}
    Behavior _behavior;
    Vector2f _lastKnownPlayerState;
  };
}
