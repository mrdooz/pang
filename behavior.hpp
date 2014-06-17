#pragma once
#include "level.hpp"


namespace pang
{
  struct Entity;

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

  struct WanderDebugRenderer : public DebugRenderer
  {
    WanderDebugRenderer(const Entity* entity) : DebugRenderer(entity) {}
    virtual void Render(RenderWindow* window);
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

  Vector2f BehaviorSeek(const Entity* e, const Vector2f& dest);
  Vector2f BehaviorArrive(const Entity* e, const Vector2f& dest);
  Vector2f BehaviorPursuit(const Entity* e, const Entity* target);

  Vector2f BehaviorWander(const Entity* e);
  Vector2f BehaviorAvoidWall(const Entity* e, const Level::Cell& cell);
}