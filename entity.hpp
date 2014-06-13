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
    Entity(EntityId id, const Vector2f& pos);
    Vector2f Dir() const;
    EntityId _id;
    Vector2f _pos;
    Vector2f _prevPos;
    Vector2f _acc;
    Vector2f _force;
    float _mass;
    float _invMass;
    // 0 points straight up, and rotates clockwise. In SFML, (0,-1) points straight up
    float _rot;
    ptime _lastAction;
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
