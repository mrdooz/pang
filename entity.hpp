#pragma once
#include "pos.hpp"
#include "types.hpp"

namespace pang
{
  struct Entity;

  struct DebugRenderer
  {
    DebugRenderer(const Entity* entity) : _entity(entity) {}
    virtual ~DebugRenderer() {};
    const Entity* _entity;
    virtual void Render(RenderWindow* window) = 0;
  };

  struct PursuitDebugRenderer : public DebugRenderer
  {
    PursuitDebugRenderer(const Entity* entity) : DebugRenderer(entity) {}
    virtual void Render(RenderWindow* window);
    Vector2f _lookAhead;
  };

  enum class Behavior
  {
    Unknown,
    Seek,
    Arrive,
    Pursuit,
    Wander,
  };

  struct AiState
  {
    AiState() : _behavior(Behavior::Unknown) {}
    Behavior _behavior;
    Vector2f _lastKnownPlayerState;
  };

  struct WanderState
  {
    WanderState() { memset(this, 0, sizeof(WanderState)); }
    float _circleOffset;
    float _circleRadius;
    float _curAngle;
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

    vector<EntityId> _visibleEntities;
  };
}
