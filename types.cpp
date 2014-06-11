#include "types.hpp"

using namespace pang;

//----------------------------------------------------------------------------------
bool pang::operator==(const Tile& lhs, const Tile& rhs)
{
  return lhs.x == rhs.x && lhs.y == rhs.y;
}
