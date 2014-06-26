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

  CreateTexture();
  CalcWallDistance();

  return true;
}

struct Room;
struct Partition
{
  enum Location
  {
    North,
    South,
    East,
    West,
  };

  enum Corner
  {
    TopLeft, TopRight, BottomLeft, BottomRight,
  };

  Partition(const sf::IntRect& bounds)
      : _bounds(bounds)
  {
    memset(_rooms, 0, sizeof(_rooms));
    memset(_partitions, 0, sizeof(_partitions));
  }

  sf::IntRect _bounds;
  Room* _rooms[4];
  Partition* _partitions[4];
};

struct Room
{
  Room(u32 id) : _id(id) {}
  u32 _id;
  sf::IntRect _bounds;
};

//----------------------------------------------------------------------------------
struct Generator
{
  ~Generator();
  // level generator based on: http://www.moddb.com/games/frozen-synapse/news/frozen-synapse-procedural-level-generation
  void Run(const pang::level::Level& config);
  void RunInner(Partition* parent);
  Room* CreateRoom(Partition* parent);

  void AddPartition(Partition* parent, Partition::Location loc, const sf::IntRect& bounds);

  pang::level::Level _config;
  sf::IntRect _bounds;
  vector<Room*> _rooms;
  vector<Partition*> _partitions;
};

//----------------------------------------------------------------------------------
Generator::~Generator()
{
  for (Room* r : _rooms)
    delete r;

  for (Partition* p : _partitions)
    delete p;
}

//----------------------------------------------------------------------------------
void Generator::Run(const pang::level::Level& config)
{
  _config = config;

  // the bounds are retracted to allow for a 1 pixel wall
  _bounds = sf::IntRect(1, 1, config.width() - 3, config.height() - 3);

  // add the global partition
  _partitions.push_back(new Partition(_bounds));
  RunInner(_partitions.back());
}

//----------------------------------------------------------------------------------
void Generator::RunInner(Partition* parent)
{
  if (/*_rooms.size() >= _config.num_rooms()*/ false
      || parent->_bounds.width <= _config.min_room_width()
      || parent->_bounds.height <= _config.min_room_height())
  {
    return;
  }

  _rooms.push_back(CreateRoom(parent));
  for (int i = 0; i < 4; ++i)
  {
    if (Partition* p = parent->_partitions[i])
    {
      RunInner(p);
    }
  }
}

//----------------------------------------------------------------------------------
void Generator::AddPartition(Partition* parent, Partition::Location loc, const sf::IntRect& bounds)
{
  Partition* p = new Partition(bounds);
  _partitions.push_back(p);
  parent->_partitions[loc] = p;
}

//----------------------------------------------------------------------------------
Room* Generator::CreateRoom(Partition* parent)
{
  // create a room inside the given bounds
  u32 id = _rooms.size();
  Room* room = new Room(id);

  int width = randf(_config.min_room_width(), min(_config.max_room_width(), parent->_bounds.width));
  int height = randf(_config.min_room_height(), min(_config.max_room_height(), parent->_bounds.height));
  room->_bounds.width = width;
  room->_bounds.height = height;

  // extract bounding dimensions
  int bleft   = parent->_bounds.left;
  int bright  = bleft + parent->_bounds.width;
  int btop    = parent->_bounds.top;
  int bbottom = btop + parent->_bounds.height;
  int bwidth  = parent->_bounds.width;
  int bheight = parent->_bounds.height;
  int rwidth  = bwidth - width;
  int rheight = bheight - height;

  // choose starting corner
  switch (rand() % 4)
  {
    // top left
    case 0:
      parent->_rooms[Partition::TopLeft] = room;
      room->_bounds.top = btop;
      room->_bounds.left = bleft;
      AddPartition(parent, Partition::South, sf::IntRect(bleft, btop + height, width, rheight));
      AddPartition(parent, Partition::East, sf::IntRect(bleft + width, btop, rwidth, bheight));
      break;

    // top right
    case 1:
      parent->_rooms[Partition::TopRight] = room;
      room->_bounds.top = btop;
      room->_bounds.left = bright - width + 1;
      AddPartition(parent, Partition::West, sf::IntRect(bleft, btop, rwidth, bheight));
      AddPartition(parent, Partition::South, sf::IntRect(bright - width, btop + height, width, rheight));
      break;

    // bottom left
    case 2:
      parent->_rooms[Partition::BottomLeft] = room;
      room->_bounds.top = bbottom - height;
      room->_bounds.left = bleft;
      AddPartition(parent, Partition::North, sf::IntRect(bleft, btop, width, rheight));
      AddPartition(parent, Partition::East, sf::IntRect(bleft + width, btop, rwidth, bheight));
      break;

    // bottom right
    case 3:
      parent->_rooms[Partition::BottomRight] = room;
      room->_bounds.top = bbottom - height;
      room->_bounds.left = bright - width;
      AddPartition(parent, Partition::West, sf::IntRect(bleft, btop, rwidth, bheight));
      AddPartition(parent, Partition::North, sf::IntRect(bright - width, btop, width, rheight));
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

#if 0
  for (const Room* r : gen._rooms)
  {
    // top
    int left    = r->_bounds.left - 1;
    int right   = r->_bounds.left + r->_bounds.width + 1;
    int top     = r->_bounds.top - 1;
    int bottom  = r->_bounds.top + r->_bounds.height + 1;

    // horiz
    for (int i = left; i < right; ++i)
    {
      SetTerrain(i, top, 1);
      SetTerrain(i, bottom, 1);
    }
    //SetTerrain(randf(left, right), top, 0);
    //SetTerrain(randf(left, right), bottom, 0);

    // vert
    for (int i = top; i < bottom; ++i)
    {
      SetTerrain(left, i, 1);
      SetTerrain(right, i, 1);
    }

    //SetTerrain(left, randf(top, bottom), 0);
    //SetTerrain(right, randf(top, bottom), 0);
  }
#endif
  for (const Room* r : gen._rooms)
  {
    // top
    int left    = r->_bounds.left;
    int right   = r->_bounds.left + r->_bounds.width;
    int top     = r->_bounds.top;
    int bottom  = r->_bounds.top + r->_bounds.height;

    Color col(rand() % 255, rand() % 255, rand() % 255);
    AddRect(left, top, right, bottom, col, r->_id);
  }

  CalcAdjacency();

  return true;
}

//----------------------------------------------------------------------------------
void Level::CalcAdjacency()
{
  const auto& sortedPair = [](u32 a, u32 b) { return make_pair(min(a, b), max(a, b)); };

  for (int i = 0; i < _height-1; ++i)
  {
    for (int j = 0; j < _width-1; ++j)
    {
      const Cell& c0 = _data[(i+0)*_width+(j+0)];
      const Cell& cx = _data[(i+0)*_width+(j+1)];
      const Cell& cy = _data[(i+1)*_width+(j+0)];

      u32 r0 = c0.roomId;
      u32 rx = cx.roomId;
      u32 ry = cy.roomId;

      // check if the current pixel is a door candidate
      if (r0 != rx)
        _connections[sortedPair(r0, rx)].vert.push_back(Vector2i(j, i));

      if (r0 != ry)
        _connections[sortedPair(r0, ry)].horiz.push_back(Vector2i(j, i));
    }
  }

  // sort the connection pixels
  for (auto& kv : _connections)
  {
    vector<Vector2i>& h = kv.second.horiz;
    sort(h.begin(), h.end(), [&](const Vector2i& lhs, const Vector2i& rhs) { return lhs.y * _width + lhs.x < rhs.y * _width + rhs.x; });

    // note, we use 'x' as the major coordinate here to get pixels in the same vertical
    // line grouped together
    vector<Vector2i>& v = kv.second.vert;
    sort(v.begin(), v.end(), [&](const Vector2i& lhs, const Vector2i& rhs) { return lhs.x * _width + lhs.y < rhs.x * _width + rhs.y; });
  }

  // fill in the walls
  for (const auto& kv : _connections)
  {
    {
      const vector<Vector2i>& v = kv.second.vert;
      int doorPos = v.size() >= 4 ? randf<u32>(1u, v.size()-2) : -100;
      for (u32 i = 0; i < v.size(); ++i)
      {
        int x = v[i].x;
        int y = v[i].y;

        if (x > 0 && x < _width - 1 && (i == doorPos || i == doorPos + 1))
          continue;

        _data[y*_width+x].col = Color::White;
      }
    }
    {
      const vector<Vector2i>& v = kv.second.horiz;
      int doorPos = v.size() >= 4 ? randf<u32>(1u, v.size()-3) : -100;
      for (u32 i = 0; i < v.size(); ++i)
      {
        int x = v[i].x;
        int y = v[i].y;

        if (y > 0 && y < _height - 1 && (i == doorPos || i == doorPos + 1))
            continue;

        _data[y*_width+x].col = Color::White;
      }

      // fill the corner
      if (!v.empty())
      {
        int x = v[0].x;
        int y = v[0].y;
        if (x > 0 && _data[y*_width+x-1].roomId == ~0)
        {
          _data[y*_width+x-1].col = Color::White;
        }
      }
    }
  }

}


//----------------------------------------------------------------------------------
void Level::AddRect(int x0, int y0, int x1, int y1, const Color& color, u32 roomId)
{
  for (int i = y0; i < y1; ++i)
  {
    for (int j = x0; j < x1; ++j)
    {
      Cell& c = _data[i*_width+j];
      c.col = color;
      c.roomId = roomId;
    }
  }
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
      Cell& cell = _data[i*_width+j];
      cell.terrain = cell.col == Color::White ? 1 : 0;
      *p++ = cell.col;
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
