#include "entity.hpp"
#include "utils.hpp"
#include "sfml_helpers.hpp"

using namespace pang;

//----------------------------------------------------------------------------------
Entity::Entity(EntityId id, const Vector2f& pos)
    : _id(id)
    , _pos(pos)
    , _prevPos(pos)
    , _vel(0,0)
    , _acc(0,0)
    , _force(0,0)
    , _debug(nullptr)
    , _mass(1.0f + (float)rand() / RAND_MAX)
    , _invMass(1/_mass)
    , _rot(0)
    , _fov(PI / 6)
    , _viewDistance(100)
{
}

//----------------------------------------------------------------------------------
Entity::~Entity()
{
  delete exch_null(_debug);
}

//----------------------------------------------------------------------------------
Vector2f Entity::Dir() const
{
  return Vector2f(sinf(_rot), -cosf(_rot));
}

