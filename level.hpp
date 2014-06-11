#pragma once

#include "types.hpp"

namespace pang
{
  namespace config
  {
    class Game;
  }

  struct Level
  {
    // player ids are 16 bit, highest bit indicates if it's a player or a bot
    //

    struct Cell
    {
      u8 terrain;
      u16 entityId;
      u16 destEntityId;
      Cell() { memset(this, 0, sizeof(Cell)); }
    };

    Level() : _width(0), _height(0) {}

    bool IsValidPos(const Tile& tile) const;
    void Init(const config::Game& config);

    bool SetEntity(const Tile& tile, u16 entityId);
    bool GetEntity(const Tile& tile, u16* entityId) const;
    void CreateTexture();

    void GetSize(u32* width, u32* height) const;
    const Texture& GetTexture() const;

    bool GetCell(const Tile& tile, Cell** cell);

  private:
    bool SetTerrain(u32 x, u32 y, u8 v);
    bool GetTerrain(u32 x, u32 y, u8* v) const;
    bool SetEntity(u32 x, u32 y, u16 entityId);
    bool GetEntity(u32 x, u32 y, u16* entityId) const;
    bool Idx(u32 x, u32 y, u32* idx) const;
    bool Idx(u32 x, u32 y, const function<void(u32)>& fn) const;

    Texture _texture;
    u32 _width, _height;
    vector<Cell> _data;
  };

}