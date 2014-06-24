#pragma once

#include "types.hpp"
#include "protocol/level.pb.h"

namespace pang
{
  namespace config
  {
    class Game;
  }

  struct Level
  {
    struct Cell
    {
      u64 wallDist; // 16 bits for N, S, W, E
      u16 entityId;
      u8 terrain;
      u8 heat;
      u8 newHeat;
      Cell() { memset(this, 0, sizeof(Cell)); }
      u16 GetWallDistN() const { return (wallDist >> 48) & 0xffff; }
      u16 GetWallDistS() const { return (wallDist >> 32) & 0xffff; }
      u16 GetWallDistW() const { return (wallDist >> 16) & 0xffff; }
      u16 GetWallDistE() const { return (wallDist >>  0) & 0xffff; }
    };

    Level() : _width(0), _height(0) {}

    bool IsVisible(u32 x0, u32 y0, u32 x1, u32 y1) const;
    bool IsValidPos(const Tile& tile) const;
    bool Init(const config::Game& config);

    bool SetEntity(const Tile& tile, u16 entityId);
    bool GetEntity(const Tile& tile, u16* entityId) const;
    void CreateTexture();
    void UpdateTexture();
    void Diffuse();

    void GetSize(u32* width, u32* height) const;
    const Texture& GetTexture() const;

    bool GetCell(const Tile& tile, Cell** cell);

  private:
    bool GenerateLevel();
    bool SetTerrain(u32 x, u32 y, u8 v);
    bool GetTerrain(u32 x, u32 y, u8* v) const;
    bool SetEntity(u32 x, u32 y, u16 entityId);
    bool GetEntity(u32 x, u32 y, u16* entityId) const;
    bool Idx(u32 x, u32 y, u32* idx) const;
    bool Idx(u32 x, u32 y, const function<void(u32)>& fn) const;
    void CalcWallDistance();

    Texture _texture;
    u32 _width, _height;
    vector<Cell> _data;
    pang::level::Level _levelConfig;

  };
}
