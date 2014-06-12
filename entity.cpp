#include "entity.hpp"

using namespace pang;

//----------------------------------------------------------------------------------
Entity::Entity()
    : _rot(0)
    , _vel(0)
    , _fov(PI / 6)
    , _viewDistance(100)
{
}

//----------------------------------------------------------------------------------
Vector2f Entity::Dir() const
{
  return Vector2f(sinf(_rot), -cosf(_rot));
}
