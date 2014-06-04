#pragma once

namespace pang
{
  namespace config
  {
    class Game;
  }

  struct Level
  {
    enum CellType : u8
    {
      CellEmpty   = 0,
      CellTerrain = 1 << 1,
      CellBullet  = 1 << 2,
      CellBomb    = 1 << 3
    };

    // player ids are 16 bit, highest bit indicates if it's a player or a bot
    //

    struct Cell
    {
      CellType type : 8;
      u16 ownerId : 16;
      union {
        u8 terrainType : 8;
        u8 bombCountdown : 8;
      };

      Cell() { memset(this, 0, sizeof(Cell)); }
    };

    Level() : _width(0), _height(0) {}

    void Init(const config::Game& config);
    bool SetTerrain(u32 x, u32 y, u8 v);
    bool GetTerrain(u32 x, u32 y, u8* v) const;

    bool SetPlayer(u32 x, u32 y, u16 playerId);
    bool GetPlayer(u32 x, u32 y, u16* playerId) const;
    void CreateTexture();

    bool Idx(u32 x, u32 y, u32* idx) const;
    bool Idx(u32 x, u32 y, const function<void(u32)>& fn);

    Texture _texture;
    u32 _width, _height;
    vector<Cell> _data;
  };

}