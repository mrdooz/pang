#pragma once

namespace pang
{
  namespace config
  {
    class Game;
  }

  struct Level
  {
    Level() : _width(0), _height(0) {}

    void Init(const config::Game& config);
    void Set(u32 x, u32 y, u8 v);
    u8 Get(u32 x, u32 y) const;
    void CreateTexture();

    bool Idx(u32 x, u32 y, u32* idx) const;

    Texture _texture;
    u32 _width, _height;
    vector<u8> _data;
  };

}