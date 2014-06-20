#pragma once

namespace pang
{

  typedef u16 EntityId;
  typedef u16 SquadId;

  struct Tile
  {
    Tile() {}
    Tile(u32 x, u32 y) : x(x), y(y), subX(0), subY(0) {}
    Tile(u32 x, u32 y, u32 subX, u32 subY) : x(x), y(y), subX(subX), subY(subY) {}
    u32 x, y;
    u32 subX, subY;
  };

  bool operator==(const Tile& lhs, const Tile& rhs);
}
