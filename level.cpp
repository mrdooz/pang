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
  for (u32 i = 0; i < config.num_walls(); ++i)
  {
    Vector2i c(rand() % _width, rand() % _height);

    // flood fill out from the center block
    u32 s = rand() % (u32)(config.max_wall_size() * min(_width, _height));
    Vector2i dir = ofs[rand() % 4];
    for (u32 j = 0; j < s; ++j)
    {
      if (Get(c.x, c.y) == 0)
      {
        Set(c.x, c.y, 1);
      }
      c += dir;
    }
  }

  CreateTexture();
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
void Level::SetPlayer(u32 x, u32 y, u32 playerId)
{
  u32 idx;
  if (Idx(x, y, &idx))
    _data[idx].background = playerId;
}

//----------------------------------------------------------------------------------
u32 Level::GetPlayer(u32 x, u32 y) const
{

}

//----------------------------------------------------------------------------------
void Level::SetBackground(u32 x, u32 y, u8 v)
{
  u32 idx;
  if (Idx(x, y, &idx))
    _data[idx].background = v;
}

//----------------------------------------------------------------------------------
u8 Level::GetBackground(u32 x, u32 y) const
{
  u32 idx;
  return Idx(x, y, &idx) ? _data[idx].background : 0xff;
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
      *p++ = _data[i*_width+j].background ? Color::White : Color::Black;
    }
  }

  _texture.update((const u8*)pixels.data());
}
