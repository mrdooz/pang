#pragma once

namespace pang
{
  #if 0
  struct Pos
  {
    Pos() {}
    Pos(s32 x, s32 y) : x(x), y(y) {}
    Pos(s16 x_hi, s16 x_lo, s16 y_hi, s16 y_lo) : x_lo(x_lo), x_hi(x_hi), y_lo(y_lo), y_hi(y_hi) {}
    union
    {
      struct
      {
        s16 x_lo;
        s16 x_hi;
      };
      s32 x;
    };

    union
    {
      struct
      {
        s16 y_lo;
        s16 y_hi;
      };
      s32 y;
    };
  };

  Pos operator-(const Pos& lhs, const Pos& rhs);
  Pos operator+(const Pos& lhs, const Pos& rhs);
  Pos operator*(float s, const Pos& p);

  bool EqualHi(const Pos& lhs, const Pos& rhs);
#endif
}

