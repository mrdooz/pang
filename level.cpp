#include "level.hpp"
#include "protocol/game.pb.h"
#include "utils.hpp"

using namespace pang;


//----------------------------------------------------------------------------------
bool Level::Init(const config::Game& config)
{
  _width = config.width();
  _height = config.height();

  _data.resize(_width * _height);

  if (!GenerateLevel())
    return false;
/*
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
*/
  CalcWallDistance();
  CreateTexture();

  return true;
}

struct Room
{
  u32 _id;
  sf::IntRect _bounds;
};

//----------------------------------------------------------------------------------
struct Generator
{
  // level generator based on: http://www.moddb.com/games/frozen-synapse/news/frozen-synapse-procedural-level-generation
  void Run(const pang::level::Level& config);
  void RunInner(const sf::IntRect& bounds);
  Room CreateRoom(const sf::IntRect& bounds, sf::IntRect* leftBounds, sf::IntRect* rightBounds);

  pang::level::Level _config;
  sf::IntRect _bounds;
  vector<Room> _rooms;
};

//----------------------------------------------------------------------------------
void Generator::RunInner(const sf::IntRect& bounds)
{
  if (_rooms.size() >= _config.num_rooms() || bounds.width < _config.min_room_width() || bounds.height < _config.min_room_height())
    return;

  sf::IntRect left, right;
  _rooms.push_back(CreateRoom(bounds, &left, &right));
  RunInner(left);
  RunInner(right);
}

//----------------------------------------------------------------------------------
void Generator::Run(const pang::level::Level& config)
{
  _config = config;

  // left, top, width, height
  _bounds = sf::IntRect(0, 0, config.width(), config.height());
  RunInner(_bounds);
}

//----------------------------------------------------------------------------------
Room Generator::CreateRoom(const sf::IntRect& bounds, sf::IntRect* leftBounds, sf::IntRect* rightBounds)
{
  // create a room inside the given bounds
  Room room;

  int width = randf(_config.min_room_width(), min(_config.max_room_width(), bounds.width));
  int height = randf(_config.min_room_height(), min(_config.max_room_height(), bounds.height));
  room._bounds.width = width;
  room._bounds.height = height;

  todo: add the room id to a lookup table in the bounds to be able to find adjacent rooms easily

  // choose starting corner
  switch (rand() % 4)
  {
    // top left
    case 0:
      room._bounds.top = bounds.top;
      room._bounds.left = bounds.left;
      *leftBounds = sf::IntRect(bounds.left, bounds.top + height - 1, width, bounds.height - height + 1);
      *rightBounds = sf::IntRect(bounds.left + width - 1, bounds.top, bounds.width - width + 1, bounds.height);
      break;

    // top right
    case 1:
      room._bounds.top = bounds.top;
      room._bounds.left = bounds.left + bounds.width - width;
      *leftBounds = sf::IntRect(bounds.left, bounds.top, bounds.width - width + 1, bounds.height);
      *rightBounds = sf::IntRect(room._bounds.left, bounds.top + height - 1, width, bounds.height - height + 1);
      break;

    // bottom left
    case 2:
      room._bounds.top = bounds.top + bounds.height - height;
      room._bounds.left = bounds.left;
      *leftBounds = sf::IntRect(bounds.left, bounds.top, width, bounds.height - height + 1);
      *rightBounds = sf::IntRect(bounds.left + width - 1, bounds.top, bounds.width - width + 1, bounds.height);
      break;

    // bottom right
    case 3:
      room._bounds.top = bounds.top + bounds.height - height;
      room._bounds.left = bounds.left + bounds.width - width;
      *leftBounds = sf::IntRect(bounds.left, bounds.top, bounds.width - width + 1, bounds.height);
      *rightBounds = sf::IntRect(room._bounds.left, bounds.top, width, bounds.height - height + 1);
      break;
  }

  return room;
}

//----------------------------------------------------------------------------------
bool Level::GenerateLevel()
{
#ifdef WIN32
  string base("d:/projects/pang/");
#else
  string base("/Users/dooz/projects/pang/");
#endif

  if (!LoadProto((base + "config/level1.pb").c_str(), &_levelConfig))
    return false;

  Generator gen;
  gen.Run(_levelConfig);

  for (const Room& r : gen._rooms)
  {
    // top
    int left = r._bounds.left;
    int right = r._bounds.left + r._bounds.width - 1;
    int top = r._bounds.top;
    int bottom = r._bounds.top + r._bounds.height - 1;

    // horiz
    for (int i = left; i < right; ++i)
    {
      SetTerrain(i, top, 1);
      SetTerrain(i, bottom, 1);
    }
    SetTerrain(randf(left, right), top, 0);
    SetTerrain(randf(left, right), bottom, 0);

    // vert
    for (int i = top; i < bottom; ++i)
    {
      SetTerrain(left, i, 1);
      SetTerrain(right, i, 1);
    }

    SetTerrain(left, randf(top, bottom), 0);
    SetTerrain(right, randf(top, bottom), 0);
  }

  return true;
}

//----------------------------------------------------------------------------------
bool Level::IsVisible(u32 x0, u32 y0, u32 x1, u32 y1) const
{
  // Bresenham from (x0, y0) to (x1, y1), and returns false if any wall is found
  // along the way

  int dx = abs(x1-x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = abs(y1-y0);
  int sy = y0 < y1 ? 1 : -1;

  const Cell* ptr = &_data[x0 + y0 * _width];

  int sPtrY = sy * (int)_width;

  if (dx > dy)
  {
    int ofs = 0;
    int threshold = dx;
    while (true)
    {
      if (ptr->terrain > 0)
        return false;

      if (x0 == x1)
        break;

      ofs += 2 * dy;
      if (ofs >= threshold)
      {
        y0 += sy;
        ptr += sPtrY;
        threshold += 2 * dx;
      }
      x0 += sx;
      ptr += sx;
    }
  }
  else
  {
    int ofs = 0;
    int threshold = dy;
    while (true)
    {
      if (ptr->terrain > 0)
        return false;

      if (y0 == y1)
        break;

      ofs += 2 * dx;
      if (ofs >= threshold)
      {
        x0 += sx;
        ptr += sx;
        threshold += 2 * dy;
      }
      y0 += sy;
      ptr += sPtrY;
    }
  }
  return true;
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
