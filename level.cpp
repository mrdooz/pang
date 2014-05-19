#include "level.hpp"

using namespace pang;

//----------------------------------------------------------------------------------
void Level::Init(u32 width, u32 height)
{
  _width = width;
  _height = height;

  _data.resize(_width * _height, 0);

  // create some random blocks :)
  for (u32 i = 0; i < 10; ++i)
  {
    u32 x = rand() % width;
    u32 y = rand() % height;
    Set(x, y, 1);
  }

  CreateTexture();
}

//----------------------------------------------------------------------------------
bool Level::Idx(u32 x, u32 y, u32* idx)
{
  if (x >= _width || y >= _height)
    return false;

  *idx = y * _width + x;
  return true;
}

//----------------------------------------------------------------------------------
void Level::Set(u32 x, u32 y, u8 v)
{
  u32 idx;
  if (Idx(x, y, &idx))
    _data[idx] = v;
}

//----------------------------------------------------------------------------------
u8 Level::Get(u32 x, u32 y)
{
  u32 idx;
  return Idx(x, y, &idx) ? _data[idx] : 0xff;
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
      *p++ = _data[i*_width+j] ? Color::White : Color::Black;
    }
  }

  _texture.update((const u8*)pixels.data());
}
