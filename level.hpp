#pragma once

namespace pang
{
  struct Level
  {
    Level() : _width(0), _height(0) {}

    void Init(u32 width, u32 height);
    void Set(u32 x, u32 y, u8 v);
    u8 Get(u32 x, u32 y);
    void CreateTexture();

    bool Idx(u32 x, u32 y, u32* idx);

    Texture _texture;
    u32 _width, _height;
    vector<u8> _data;
  };

}