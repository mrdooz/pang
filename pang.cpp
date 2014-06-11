#include "pang.hpp"
#include "window_event_manager.hpp"
#include "action.hpp"
#include "utils.hpp"
#include "sfml_helpers.hpp"

using namespace pang;

template <typename T>
bool LoadProto(const char* filename, T* out)
{
  FILE* f = fopen(filename, "rb");
  if (!f)
    return false;

  fseek(f, 0, 2);
  size_t s = ftell(f);
  fseek(f, 0, 0);
  string str;
  str.resize(s);
  fread((char*)str.c_str(), 1, s, f);
  fclose(f);

  return google::protobuf::TextFormat::ParseFromString(str, out);
}

void Line(int x0, int y0, int x1, int y1, vector<Vector2i>* line)
{
  line->clear();

  // Bresenham between the points
  int dx = abs(x1-x0);
  int sx = x0 < x1 ? 1 : -1;
  int dy = abs(y1-y0);
  int sy = y0 < y1 ? 1 : -1;

  if (dx > dy)
  {
    int ofs = 0;
    int threshold = dx;
    while (true)
    {
      line->push_back(Vector2i(x0,y0));
      if (x0 == x1)
        break;

      ofs += 2 * dy;
      if (ofs >= threshold)
      {
        y0 += sy;
        threshold += 2 * dx;
      }
      x0 += sx;
    }
  }
  else
  {
    int ofs = 0;
    int threshold = dy;
    while (true)
    {
      line->push_back(Vector2i(x0,y0));
      if (y0 == y1)
        break;

      ofs += 2 * dx;
      if (ofs >= threshold)
      {
        x0 += sx;
        threshold += 2 * dy;
      }
      y0 += sy;
    }
  }
}


//----------------------------------------------------------------------------------
Game::Game()
    : _gridSize(25)
    , _focus(true)
    , _done(false)
    , _localPlayerId(1)
    , _prevLeft(0)
    , _prevRight(0)
    , _debugDraw(0)
    , _playerDead(false)
    , _pausedEnemies(true)
{
}

//----------------------------------------------------------------------------------
bool Game::Init()
{
  size_t width, height;
#ifdef _WIN32
  width = GetSystemMetrics(SM_CXFULLSCREEN);
  height = GetSystemMetrics(SM_CYFULLSCREEN);
#else
  auto displayId = CGMainDisplayID();
  width = CGDisplayPixelsWide(displayId);
  height = CGDisplayPixelsHigh(displayId);
#endif

  sf::ContextSettings settings;
  _renderWindow.reset(new RenderWindow(sf::VideoMode(8 * width / 10, 8 * height / 10), "...", sf::Style::Default, settings));
  _renderWindow->setVerticalSyncEnabled(true);
  _eventManager.reset(new WindowEventManager(_renderWindow.get()));

  _eventManager->RegisterHandler(Event::KeyPressed, bind(&Game::OnKeyPressed, this, _1));
  _eventManager->RegisterHandler(Event::KeyReleased, bind(&Game::OnKeyReleased, this, _1));
  _eventManager->RegisterHandler(Event::LostFocus, bind(&Game::OnLostFocus, this, _1));
  _eventManager->RegisterHandler(Event::GainedFocus, bind(&Game::OnGainedFocus, this, _1));
  _eventManager->RegisterHandler(Event::MouseButtonReleased, bind(&Game::OnMouseButtonReleased, this, _1));

#ifdef WIN32
  string base("d:/projects/pang/");
#else
  string base("/Users/dooz/projects/pang/");
#endif
  if (!_font.loadFromFile(base + "gfx/04b_03b_.ttf"))
  {
    return false;
  }

  if (!LoadProto((base + "config/game.pb").c_str(), &_gameConfig))
    return 1;

  _level.Init(_gameConfig);

  // create local player
  shared_ptr<Entity> e = make_shared<Entity>();
  e->_id = _localPlayerId;
  e->_pos = GetEmptyPos();
  _entities[_localPlayerId] = e;

  _level.SetEntity(WorldToTile(e->_pos), e->_id);

  SpawnEnemies();

  return true;
}

//----------------------------------------------------------------------------------
Vector2f Game::GetEmptyPos() const
{
  u32 w, h;
  _level.GetSize(&w, &h);
  while (true)
  {
    Vector2f pos(rand() % w, rand() % h);
    if (_level.IsValidPos(Tile(pos.x, pos.y)))
    {
      return (float)_gridSize * pos;
    }
  }
  return Vector2f(0,0);
}

//----------------------------------------------------------------------------------
void Game::SpawnEnemies()
{
  for (u32 i = 0; i < _gameConfig.num_enemies(); ++i)
  {
    EntityId idx = _localPlayerId + 1 + i;
    shared_ptr<Entity> e = make_shared<Entity>();
    e->_id = idx;
    e->_pos = GetEmptyPos();
    _entities[idx] = e;

    _level.SetEntity(WorldToTile(e->_pos), e->_id);
  }
}

Vector2f Normaize(const Vector2f& v)
{
  float len = sqrtf(v.x * v.x + v.y * v.y);
  if (len == 0)
    return Vector2f(0,0);

  return 1 / len * v;
}

float Dot(const Vector2f& a, const Vector2f& b)
{
  return a.x * b.x + a.y * b.y;
}

//----------------------------------------------------------------------------------
u32 Game::ActionScore(const Entity& entity, const AiState& aiState)
{
  // check if the player is within the viewing cone
  const Entity* player = _entities[_localPlayerId].get();

  // check angle to player
  // dot(a,b) = cos(angle)
  //Vector2f toPlayer = Normalize(player->_pos - entity._pos);
  //float angle = acosf(Dot(toPlayer, entity._dir));

  return 0;
}


//----------------------------------------------------------------------------------
void Game::UpdateEnemies()
{
  if (_playerDead || _pausedEnemies)
    return;

  Vector2f playerPos(_entities[_localPlayerId]->_pos);

  for (auto& kv : _entities)
  {
    Entity* e = kv.second.get();
    if (e->_id == _localPlayerId)
      continue;

    // turn towards player
    Vector2f dir = (playerPos - e->_pos);
    Normalize(dir);

    float a = atan2f(dir.x, dir.y);
    e->_rot = a;

    Vector2f dest(e->_pos + (float)_gridSize * dir);
    Level::Cell* cell;
    if (_level.GetCell(WorldToTile(dest), &cell))
    {
      // don't move to an occupied cell, or one which is another entity's dest
      if ((cell->entityId && cell->entityId != e->_id) || (cell->destEntityId && cell->destEntityId != e->_id))
        continue;
    }

    AddMoveAction(e->_id, e->_pos, dest);

    if (_debugDraw & 2)
      AddMessage(MessageType::Debug, toString("dx: %.2f, dy: %.2f, a: %.2f", dir.x, dir.y, a));

    //SpawnBullet(e);
  }
}


//----------------------------------------------------------------------------------
bool Game::OnLostFocus(const Event& event)
{
  _focus = false;
  return true;
}

//----------------------------------------------------------------------------------
bool Game::OnGainedFocus(const Event& event)
{
  _focus = true;
  return true;
}

//----------------------------------------------------------------------------------
Vector2f Game::SnappedPos(const Vector2f& pos)
{
  float ff = _gridSize;
  float f = ff - 1;
  int x = (pos.x + f) / ff;
  int y = (pos.y + f) / ff;
  return Vector2f(x * ff, y * ff);
}

//----------------------------------------------------------------------------------
Vector2f Game::ClampedDestination(const Vector2f& pos, const Vector2f& dir)
{
  return SnappedPos(pos + (float)_gridSize * dir);
}

//----------------------------------------------------------------------------------
void Game::ReadKeyboard()
{
  if (_playerDead)
    return;

  Entity& e = *_entities[_localPlayerId];

  // 0 = no movement, 1 = x axis, 2 = y axis
  u32 moveAction = 0;

  u8 curLeft = Keyboard::isKeyPressed(Keyboard::Left) || Keyboard::isKeyPressed(Keyboard::A);
  u8 curRight = Keyboard::isKeyPressed(Keyboard::Right) || Keyboard::isKeyPressed(Keyboard::D);

  if (curLeft && !_prevLeft)
  {
    e._rot -= PI / 2;
    moveAction = 1;
  }
  else if (curRight && !_prevRight)
  {
    e._rot += PI / 2;
    moveAction = 1;
  }
  else if (Keyboard::isKeyPressed(Keyboard::Up) || Keyboard::isKeyPressed(Keyboard::W))
  {
    e._vel = Clamp(e._vel + 1, -1.f, 1.f);
    moveAction = 2;
  }
  else if (Keyboard::isKeyPressed(Keyboard::Down) || Keyboard::isKeyPressed(Keyboard::S))
  {
    e._vel = Clamp(e._vel - 1, -1.f, 1.f);
    moveAction = 2;
  }

  _prevLeft = curLeft;
  _prevRight = curRight;

  if (moveAction)
  {
    // calc new destination based on current pos and direction
    AddMoveAction(_localPlayerId, e._pos, ClampedDestination(e._pos, e._vel * Vector2f(sinf(e._rot), -cosf(e._rot))));
  }
}

//----------------------------------------------------------------------------------
void Game::DebugDrawEntity()
{
  if (!_selectedEntity)
    return;

  const Entity& e = *_selectedEntity;
  AddMessage(MessageType::Debug, toString("id: %d", e._id));
  AddMessage(MessageType::Debug, toString("pos: x: %.2f, y: %.2f, rot: %.2f", e._pos.x, e._pos.y, e._rot));
}


//----------------------------------------------------------------------------------
bool Game::SpawnBullet(Entity& e)
{
  if (e._lastAction.is_not_a_date_time())
  {
    e._lastAction = _now;
  }
  else if (_now - e._lastAction < seconds(1))
  {
    return false;
  }

  e._lastAction = _now;

  Vector2f c(_gridSize/2, _gridSize/2);

  Vector2f dir = Vector2f(sinf(e._rot), -cosf(e._rot));
  Vector2f pos = SnappedPos(e._pos) + c + (float)_gridSize * dir;
  if (_level.IsValidPos(WorldToTile(pos)))
  {
    ActionBullet* b = new ActionBullet();
    b->entityId = e._id;
    b->pos = pos;
    b->dir = dir;
    _actionQueue.push_back(b);
  }

  return true;
}

//----------------------------------------------------------------------------------
bool Game::OnMouseButtonReleased(const Event& event)
{
  // the view is centered around the local player, so compensate for this
  const Vector2f& p = _renderWindow->mapPixelToCoords(Vector2i(event.mouseButton.x, event.mouseButton.y));
  Tile tile = WorldToTile(p);

  _selectedEntity.reset();

  for (const auto& kv : _entities)
  {
    Tile entityTile = WorldToTile(kv.second->_pos);
    if (entityTile == tile)
    {
      _selectedEntity = kv.second;
      break;
    }
  }
  return true;
}


//----------------------------------------------------------------------------------
bool Game::OnKeyPressed(const Event& event)
{
  Keyboard::Key key = event.key.code;

  if (_playerDead)
  {
    switch (key)
    {
      case Keyboard::Escape:
        _done = true;
        break;
    }
    return true;
  }

  Entity& e = *_entities[_localPlayerId].get();

  switch (key)
  {
    case Keyboard::Space:
    {
      SpawnBullet(e);
      break;
    }

    case Keyboard::Escape:
      _done = true;
      break;
  }

  return true;
}

//----------------------------------------------------------------------------------
bool Game::OnKeyReleased(const Event& event)
{
  return true;
}

//----------------------------------------------------------------------------------
void Game::DrawGrid()
{
  _levelSprite.setPosition(0, 0);
  _levelSprite.setTexture(_level.GetTexture());
  _levelSprite.setScale(_gridSize, _gridSize);
  _renderWindow->draw(_levelSprite);

  vector<sf::Vertex> lines;
  Color c(0x80, 0x80, 0x80);

  u32 w, h;
  _level.GetSize(&w, &h);

  // horizontal
  for (u32 i = 0; i <= h; ++i)
  {
    lines.push_back(sf::Vertex(Vector2f(0, i*_gridSize), c));
    lines.push_back(sf::Vertex(Vector2f(w*_gridSize, i*_gridSize), c));
  }

  // vertical
  for (u32 i = 0; i <= w; ++i)
  {
    lines.push_back(sf::Vertex(Vector2f(i*_gridSize, 0), c));
    lines.push_back(sf::Vertex(Vector2f(i*_gridSize, h*_gridSize), c));
  }

  _renderWindow->draw(lines.data(), lines.size(), sf::Lines);

}

//----------------------------------------------------------------------------------
void Game::AddMoveAction(EntityId entityId, const Vector2f& from, const Vector2f& to)
{
  if (!_level.IsValidPos(WorldToTile(from)) || !_level.IsValidPos(WorldToTile(to)))
  {
    _entities[entityId]->_vel = 0;
    return;
  }

  // check if a move action to the given location is already in progress
  for (auto it = _inprogressActions.begin(); it != _inprogressActions.end(); ++it)
  {
    ActionBase* a = *it;
    if (a->type == ActionType::Move && a->entityId == entityId)
    {
      ActionMove* m = static_cast<ActionMove*>(a);
      if (SnappedPos(m->to) == SnappedPos(to))
      {
        // The requested move is already in progress, so bail
        return;
      }
    }
  }

  ActionMove* m = nullptr;
  for (ActionBase* a : _actionQueue)
  {
    if (a->type == ActionType::Move && a->entityId == entityId)
    {
      // if a move action for the current player exists in the queue, replace it
      ActionMove* aa = static_cast<ActionMove*>(a);
      Level::Cell* oldCell;
      _level.GetCell(WorldToTile(aa->to), &oldCell);
      oldCell->destEntityId = 0;
      m = aa;
      break;
    }
  }

  // Create the new action if needed
  if (!m)
  {
    m = new ActionMove();
    m->entityId = entityId;
    _actionQueue.push_back(m);
  }

  Level::Cell* cell;
  _level.GetCell(WorldToTile(to), &cell);
  cell->destEntityId = entityId;

  m->from = from;
  m->to = to;
  m->startTime = _now;
  m->endTime = _now + milliseconds(500);
}

//----------------------------------------------------------------------------------
void Game::EraseMoveActions(EntityId entityId)
{
  for (auto it = _inprogressActions.begin(); it != _inprogressActions.end();)
  {
    ActionBase* a = *it;
    if (a->type == ActionType::Move && a->entityId == entityId)
    {
      delete a;
      it = _inprogressActions.erase(it);
    }
    else
    {
      ++it;
    }
  }
}
//----------------------------------------------------------------------------------
void Game::Update()
{
  _now = microsec_clock::local_time();
  _eventManager->Poll();

  ReadKeyboard();
  HandleActions();

  if (_lastUpdate.is_not_a_date_time())
  {
    _lastUpdate = _now;
    return;
  }

  float delta = (_now - _lastUpdate).total_milliseconds() / 1000.0f;

  // update all the bullets
  for (auto it = _bullets.begin(); it != _bullets.end(); )
  {
    Bullet& b = *it;
    b.pos = b.pos + 100 * delta * b.dir;
    if (!_level.IsValidPos(WorldToTile(b.pos)))
    {
      it = _bullets.erase(it);
    }
    else
    {
      bool collision = false;
      // check for player collision
      for (auto j = _entities.begin(); j != _entities.end(); )
      {
        shared_ptr<Entity> e = j->second;
        if (e->_id != b.entityId && SnappedPos(e->_pos) == SnappedPos(b.pos))
        {
          collision = true;
          if (e->_id == _localPlayerId)
            _playerDead = true;
          _deadEntites[e->_id] = e;
          _entities.erase(j);
          break;
        }
        else
        {
          ++j;
        }
      }

      it = collision ? _bullets.erase(it) : ++it;
    }
  }

  UpdateEnemies();

  _lastUpdate = _now;
}

//----------------------------------------------------------------------------------
void Game::HandleActions()
{
  // Move actions from the queue to the in progress queue
  for (ActionBase* action : _actionQueue)
  {
    EntityId entityId = action->entityId;

    bool instantAction = false;

    if (action->type == ActionType::Move)
    {
      // only allow a single in progress move action, so erase any in progress for
      // the current entity
      EraseMoveActions(entityId);
    }
    else if (action->type == ActionType::Bullet)
    {
      ActionBullet* a = static_cast<ActionBullet*>(action);
      Bullet b;
      b.pos = a->pos;
      b.dir = a->dir;
      b.entityId = a->entityId;
      _bullets.push_back(b);
      instantAction = true;
    }

    // Either delete the action if it was instant, or move it to
    // _inprogressActions if it has a duration
    if (instantAction)
    {
      delete action;
    }
    else
    {
      _inprogressActions.push_back(action);
    }
  }

  _actionQueue.clear();


  // handle in progress actions
  for (auto it = _inprogressActions.begin(); it != _inprogressActions.end(); )
  {
    ActionBase* action = *it;

    bool deleteAction = false;
    if (action->type == ActionType::Move)
    {
      ActionMove* m = static_cast<ActionMove*>(action);
      Entity& e = *_entities[m->entityId];
      Vector2f prevPos = e._pos;
      // interpolate the player position
      float delta = (float)(_now - m->startTime).total_milliseconds() / (m->endTime - m->startTime).total_milliseconds();
      e._pos = m->from + delta * (m->to - m->from);

      _level.SetEntity(WorldToTile(prevPos), 0);
      _level.SetEntity(WorldToTile(e._pos), e._id);

      if (_now >= m->endTime)
      {
        deleteAction = true;
        e._vel = 0;
        e._pos = m->to;
        Level::Cell* cell;
        _level.GetCell(WorldToTile(e._pos), &cell);
        cell->destEntityId = 0;
      }
    }

    it = deleteAction ? _inprogressActions.erase(it) : ++it;
  }
}

//----------------------------------------------------------------------------------
void Game::Render()
{
  _renderWindow->clear();

  if (!_playerDead)
  {
    Vector2u s = _renderWindow->getSize();
    _view.setCenter(_entities[_localPlayerId]->_pos);
    _view.setRotation(0);
    _view.setSize(s.x, s.y);
    _renderWindow->setView(_view);
  }

  DrawGrid();
  DebugDrawEntity();

  Vector2f c(_gridSize/2, _gridSize/2);

  for (const auto& kv : _entities)
  {
    const Entity& e = *kv.second;
    VertexArray triangle(sf::Triangles, 3);
    Transform rotation;
    Color col = e._id == _localPlayerId ? Color::Green : Color::Yellow;
    rotation.rotate(180 + e._rot * 180 / PI);
    triangle[0].position = c + e._pos + rotation.transformPoint(Vector2f(0, 20));
    triangle[0].color = Color::Red;
    triangle[1].position = c + e._pos + rotation.transformPoint(Vector2f(-5, 0));
    triangle[1].color = col;
    triangle[2].position = c + e._pos + rotation.transformPoint(Vector2f(5, 0));
    triangle[2].color = col;
    _renderWindow->draw(triangle);

    ArcShape aa(e._pos, 30, 0, PI / 4);
    _renderWindow->draw(aa);

    if (e._id == _localPlayerId && (_debugDraw & 0x4))
    {
      AddMessage(MessageType::Debug, toString("x: %.2f, y: %.2f", e._pos.x, e._pos.y));
    }
  }

  if (_playerDead)
  {
    AddMessage(MessageType::Debug, "** GAME OVER **");
  }

  RectangleShape rect;
  rect.setFillColor(Color::Red);
  rect.setSize(Vector2f(6, 6));
  Vector2f ofs(0, 3);

  for (const Bullet& b : _bullets)
  {
    rect.setPosition(b.pos - ofs);
    _renderWindow->draw(rect);
  }

  if (_debugDraw & 0x1)
  {
    rect.setFillColor(Color(0x80, 0x80, 0, 0x80));
    rect.setSize(Vector2f(_gridSize, _gridSize));

    Vector2f localPos(_entities[_localPlayerId]->_pos);

    for (const auto& kv : _entities)
    {
      const Entity& e = *kv.second;
      if (e._id == _localPlayerId)
        continue;

      vector<Vector2i> p;
      Line(e._pos.x / _gridSize, e._pos.y / _gridSize, localPos.x / _gridSize, localPos.y / _gridSize, &p);

      for (const Vector2i& x : p)
      {
        rect.setPosition(_gridSize * x.x, _gridSize * x.y);
        _renderWindow->draw(rect);
      }
    }
  }

  UpdateMessages();
  _renderWindow->display();
}

//----------------------------------------------------------------------------------
bool Game::Run()
{
  while (_renderWindow->isOpen() && !_done)
  {
    Update();
    Render();
  }

  return true;
}

//------------------------------------------------------------------------------
bool Game::Close()
{
  return true;
}

//------------------------------------------------------------------------------
void Game::AddMessage(MessageType type, const string& str)
{
  Message msg;
  msg.str = str;

  if (type == MessageType::Debug)
  {
    u8 c = (u8)(255 * 0.8f);
    msg.color = Color(c, c, c);
  }
  else if (type == MessageType::Info)
  {
    u8 c = 255 * 0.8f;
    msg.color = Color(c, c, c);
    msg.endTime = microsec_clock::local_time() + seconds(5);
  }
  else if (type == MessageType::Warning)
  {
    msg.color = Color(0.9f * 255, 0.9f * 255, 0);
    msg.endTime = microsec_clock::local_time() + seconds(10);
  }
  else
  {
    // error
    msg.color = Color(0.9f * 255, 0.2f * 255, 0);
    msg.endTime = microsec_clock::local_time() + seconds(20);
  }

  _messages.push_back(msg);
}

//------------------------------------------------------------------------------
Tile Game::WorldToTile(const Vector2f& p) const
{
  return Tile(p.x / _gridSize, p.y / _gridSize);
}

//------------------------------------------------------------------------------
Vector2f Game::TileToWorld(u32 x, u32 y) const
{
  // returns a point in the center of the tile
  return Vector2f(x * _gridSize + _gridSize / 2, y * _gridSize + _gridSize / 2);
}

//------------------------------------------------------------------------------
void Game::UpdateMessages()
{
  ptime now = microsec_clock::local_time();

  Vector2f pos = _renderWindow->mapPixelToCoords(Vector2i(300, 0));
  float x = pos.x;
  float y = pos.y;
  RectangleShape rect;
  rect.setPosition(x, y);
  rect.setFillColor(Color(128, 128, 128, 128));
  rect.setSize(Vector2f(16*40, 17 * _messages.size()));
  _renderWindow->draw(rect);

  Text text;
  text.setFont(_font);
  text.setCharacterSize(16);

  for (auto it = _messages.begin(); it != _messages.end(); )
  {
    Message& msg = *it;
    if (msg.endTime.is_not_a_date_time() || msg.endTime > now)
    {
      // blend out alpha over the last second
      time_duration left = msg.endTime - now;
      if (left < seconds(1))
      {
        msg.color.a = 255 * left.total_milliseconds() / 1000.0f;
      }

      text.setPosition(x, y);
      text.setString(msg.str);
      _renderWindow->draw(text);
      y += 16;
    }

    if (!msg.endTime.is_not_a_date_time() && msg.endTime > now)
    {
      ++it;
    }
    else
    {
      // message has elapsed, so remove it
      it = _messages.erase(it);
    }
  }
}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  Game game;

  if (!game.Init())
    return 1;

  game.Run();

  if (!game.Close())
    return 1;

  return 0;
}
