#pragma once
#include "pos.hpp"
#include "types.hpp"

namespace pang
{
  struct DebugRenderer;

  struct Squad
  {
    u16 _id;
  };

  struct Entity
  {
    Entity(EntityId id, const Vector2f& pos);
    ~Entity();
    Vector2f Dir() const;
    EntityId _id;
    Vector2f _pos;
    Vector2f _prevPos;
    Vector2f _vel;
    Vector2f _acc;
    Vector2f _force;
    DebugRenderer* _debug;
    float _mass;
    float _invMass;
    // 0 points straight up, and rotates clockwise. In SFML, (0,-1) points straight up
    float _rot;
    ptime _lastAction;
    // fov is symmetric along the direction vector
    float _fov;
    float _viewDistance;
    SquadId _squadId;

    vector<EntityId> _visibleEntities;
  };
}
