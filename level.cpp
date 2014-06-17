#include "level.hpp"
#include "protocol/game.pb.h"

using namespace pang;


//----------------------------------------------------------------------------------
void Level::Init(const config::Game& config)
{
  _width = config.width();
  _height = config.height();

  _data.resize(_width * _height);

  Vector2i ofs[] = { Vector2i(-1, 0), Vector2i(1, 0), Vector2i(0, -1), Vector2i(0, 1) };

  // create some random blocks :)
  for (int i = 0; i < config.num_walls(); ++i)
  {
    Vector2i c(rand() % _width, rand() % _height);

    // flood fill out from the center block
    u32 s = rand() % (u32)(config.max_wall_size() * min(_width, _height));
    Vector2i dir = ofs[rand() % 4];
    for (u32 j = 0; j < s; ++j)
    {
      SetTerrain(c.x, c.y, 1);
      c += dir;
    }
  }

  CalcWallDistance();
  CreateTexture();
}

//----------------------------------------------------------------------------------
bool Level::IsValidPos(const Tile& tile) const
{
  u32 tmp;
  return Idx(tile.x, tile.y, &tmp);
}

//----------------------------------------------------------------------------------
bool Level::Idx(u32 x, u32 y, const function<void(u32)>& fn) const
{
  if (x >= _width || y >= _height)
    return false;
  fn(y * _width + x);
  return true;
}

//----------------------------------------------------------------------------------
bool Level::Idx(u32 x, u32 y, u32* idx) const
{
  if (x >= _width || y >= _height)
    return false;

  *idx = y * _width + x;
  return true;
}

//----------------------------------------------------------------------------------
bool Level::SetTerrain(u32 x, u32 y, u8 v)
{
  return Idx(x, y, [=](u32 idx) { _data[idx].terrain = v; });
}

//----------------------------------------------------------------------------------
bool Level::GetTerrain(u32 x, u32 y, u8* v) const
{
  return Idx(x, y, [=](u32 idx) { *v = _data[idx].terrain; });
}

//----------------------------------------------------------------------------------
bool Level::SetEntity(const Tile& tile, u16 entityId)
{
  return Idx(tile.x, tile.y, [=](u32 idx) { _data[idx].entityId = entityId; });
}

//----------------------------------------------------------------------------------
bool Level::SetEntity(u32 x, u32 y, u16 entityId)
{
  return Idx(x, y, [=](u32 idx) { _data[idx].entityId = entityId; });
}

//----------------------------------------------------------------------------------
bool Level::GetEntity(const Tile& tile, u16* entityId) const
{
  return Idx(tile.x, tile.y, [=](u32 idx) { *entityId = _data[idx].terrain; });
}

//----------------------------------------------------------------------------------
bool Level::GetEntity(u32 x, u32 y, u16* entityId) const
{
  return Idx(x, y, [=](u32 idx) { *entityId = _data[idx].terrain; });
}

//----------------------------------------------------------------------------------
void Level::GetSize(u32* width, u32* height) const
{
  if (width)
    *width = _width;

  if (height)
    *height = _height;
}

//----------------------------------------------------------------------------------
const Texture& Level::GetTexture() const
{
  return _texture;
}

//----------------------------------------------------------------------------------
bool Level::GetCell(const Tile& tile, Cell** cell)
{
  return Idx(tile.x, tile.y, [=](u32 idx) { *cell = &_data[idx]; });
}


//----------------------------------------------------------------------------------
void Level::CreateTexture()
{
  _texture.create(_width, _height);

  // create rgba texture
  vector<Color> pixels(_width * _height);
  Color* p = pixels.data();

  for (u32 i = 0; i < _height; ++i)
  {
    for (u32 j = 0; j < _width; ++j)
    {
      const Cell& cell = _data[i*_width+j];
      *p++ = cell.terrain ? Color::White : Color::Black;
#if 0
      if (cell.terrain > 0)
      {
        *p++ = Color::White;
      }
      else
      {
        *p++ = Color(max(0, 255 - 10 * cell.GetWallDistW()), 0, 0);
      }
#endif
    }
  }

  _texture.update((const u8*)pixels.data());
}

//----------------------------------------------------------------------------------
void Level::UpdateTexture()
{
  vector<Color> pixels(_width * _height);
  Color* p = pixels.data();
  Cell* cell = _data.data();
  for (u32 i = 0; i < _height; ++i)
  {
    for (u32 j = 0; j < _width; ++j)
    {
      if (cell->terrain == 0)
      {
        u8 h = cell->newHeat;
        cell->heat = cell->newHeat;
        *p = Color(h, h, h, 255);
      }
      else
      {
        *p = Color::White;
      }

      ++cell;
      ++p;
    }
  }

  _texture.update((const u8*)pixels.data());
}


//----------------------------------------------------------------------------------
void Level::Diffuse()
{
  // box filter all the cell heat
  Cell* cell = _data.data();
  for (u32 i = 1; i < _height-1; ++i)
  {
    for (u32 j = 1; j < _width-1; ++j)
    {
      static s32 ofs[] = {
          -1, -1, +0, -1, +1, -1,
          -1, +0, +0, +0, +1, +0,
          -1, +1, +0, +1, +1, +1 };

      u32 res = 0;
      for (u32 k = 0; k < 9; ++k)
      {
        res += (cell + ofs[k*2+0] + ofs[k*2+1] * (s32)_width)->heat;
      }
      res = res / 9;
      cell->newHeat = min(255u, res);
      ++cell;
    }
  }
}

//----------------------------------------------------------------------------------
void Level::CalcWallDistance()
{
  for (u32 i = 0; i < _height; ++i)
  {
    for (u32 j = 0; j < _width; ++j)
    {
      Cell& cell = _data[i*_width+j];
      cell.wallDist = 0;
      if (cell.terrain == 0)
      {
        int k;

        // N
        for (k = i - 1; k >= 0 && _data[k*_width+j].terrain == 0; --k)
          continue;
        cell.wallDist |= (u64)(i - max(0, k)) << 48;

        // S
        for (k = i + 1; k < _height && _data[k*_width+j].terrain == 0; ++k)
          continue;
        cell.wallDist |= (u64)(min((int)_height-1, k) - i) << 32;

        // W
        for (k = j - 1; k >= 0 && _data[i*_width+k].terrain == 0; --k)
          continue;
        cell.wallDist |= (u64)(j - max(0, k)) << 16;

        // E
        for (k = j + 1; k < _width && _data[i*_width+k].terrain == 0; ++k)
          continue;
        cell.wallDist |= (u64)(min((int)_width-1, k) - j) << 0;
      }
    }
  }
}
