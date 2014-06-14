#include "entity.hpp"
#include "utils.hpp"

using namespace pang;

//----------------------------------------------------------------------------------
Entity::Entity(EntityId id, const Vector2f& pos)
    : _id(id)
    , _pos(pos)
    , _prevPos(pos)
    , _vel(0,0)
    , _acc(0,0)
    , _force(0,0)
    , _lookAhead(pos)
    , _mass(1.0f + (float)rand() / RAND_MAX)
    , _invMass(1/_mass)
    , _rot(PI)
    , _fov(PI / 6)
    , _viewDistance(100)
{
}

//----------------------------------------------------------------------------------
Vector2f Entity::Dir() const
{
  return Vector2f(sinf(_rot), cosf(_rot));
}
