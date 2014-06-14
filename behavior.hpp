#pragma once

namespace pang
{
  struct Entity;
  Vector2f BehaviorSeek(const Entity* e, const Vector2f& dest);
  Vector2f BehaviorArrive(const Entity* e, const Vector2f& dest);
  Vector2f BehaviorPursuit(const Entity* e, const Entity* target);

  Vector2f BehaviorWander(Entity* e);

}