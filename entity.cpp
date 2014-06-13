#include "entity.hpp"
#include "utils.hpp"

using namespace pang;

//----------------------------------------------------------------------------------
Entity::Entity()
    : _acc(0,0)
    , _vel(0,0)
    , _rot(PI)
    , _fov(PI / 6)
    , _viewDistance(100)
{
}

//----------------------------------------------------------------------------------
Vector2f Entity::Dir() const
{
  return Vector2f(sinf(_rot), -cosf(_rot));
}
