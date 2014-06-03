#pragma once

namespace pang
{
  namespace config
  {
    class Game;
  }

  struct Level
  {
    enum CellType
    {
      CellEmpty   = 0,
      CellTerrain = 1 << 1,
      CellEntity  = 1 << 2,
      CellBullet  = 1 << 3,
      CellBomb    = 1 << 4
    };

    // player ids are 16 bit, highest bit indicates if it's a player or a bot
    //

    struct Cell
    {
      CellType type : 8;
      union {
        u32 playerId : 16;
      };

      Cell() : background(0), playerId(0) {}
      u8 background;
      u32 playerId;
    };

    Level() : _width(0), _height(0) {}

    void Init(const config::Game& config);
    bool SetBackground(u32 x, u32 y, u8 v);
    bool GetBackground(u32 x, u32 y, u8* v) const;

    bool SetPlayer(u32 x, u32 y, u32 playerId);
    bool GetPlayer(u32 x, u32 y, u32* playerId) const;
    void CreateTexture();

    bool Idx(u32 x, u32 y, u32* idx) const;

    Texture _texture;
    u32 _width, _height;
    vector<u32> _data;
  };

}