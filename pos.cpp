#include "pos.hpp"

using namespace pang;

namespace pang
{
  #if 0
  Pos operator+(const Pos& lhs, const Pos& rhs)
  {
    return Pos(lhs.x + rhs.x, lhs.y + rhs.y);
  }

  Pos operator-(const Pos& lhs, const Pos& rhs)
  {
    return Pos(lhs.x - rhs.x, lhs.y - rhs.y);
  }

  Pos operator*(float s, const Pos& p)
  {
    return Pos(s * p.x, s * p.y);
  }


  bool EqualHi(const Pos& lhs, const Pos& rhs)
  {
    return lhs.x_hi == rhs.x_hi && lhs.y_hi == rhs.y_hi;
  }
  #endif
}

