#include "behavior.hpp"
#include "entity.hpp"
#include "math_utils.hpp"

namespace pang
{
  namespace
  {
    const float MAX_FORCE = 0.0005f;
    const float MAX_SPEED = 1.0f;
  }
//----------------------------------------------------------------------------------
  Vector2f BehaviorSeek(const Entity* e, const Vector2f& dest)
  {
    // returns a force towards the dest
    Vector2f desiredVel = MAX_SPEED * Normalize(dest - e->_pos);
    return MAX_FORCE * (desiredVel - e->_vel);
  }

//----------------------------------------------------------------------------------
  Vector2f BehaviorArrive(const Entity* e, const Vector2f& dest)
  {
    float dist = Length(dest - e->_pos);
    if (dist == 0)
      return Vector2f(0,0);

    Vector2f desiredVel = MAX_SPEED * Normalize(dist * (dest - e->_pos));
    return MAX_FORCE * (desiredVel - e->_vel);
  }

  //----------------------------------------------------------------------------------
  Vector2f BehaviorPursuit(const Entity* e, const Entity* target)
  {
    Vector2f toTarget(target->_pos - e->_pos);

    float s = Length(toTarget) / (Length(e->_vel) + Length(target->_vel));
    Vector2f v = target->_pos + s * target->_vel;
    if (e->_debug)
    {
      // meh, is there a better way to do this?
      PursuitDebugRenderer* p = static_cast<PursuitDebugRenderer*>(e->_debug);
      p->_lookAhead = v;
    }
    return BehaviorSeek(e, v);
  }
}
