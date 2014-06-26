#include "pang.hpp"
#include "window_event_manager.hpp"
#include "utils.hpp"
#include "math_utils.hpp"
#include "sfml_helpers.hpp"
#include "math_utils.hpp"
#include "behavior.hpp"

using namespace pang;

//----------------------------------------------------------------------------------
Game::Game()
    : _gridSize(25)
    , _focus(true)
    , _done(false)
    , _playerDead(false)
    , _pausedEnemies(false)
    , _localPlayerId(1)
    , _prevLeft(0)
    , _prevRight(0)
    , _tickAcc(0)
{
  //_debugDraw.Set(DebugDrawFlags::DrawLevel);
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
  _windowSize = Vector2i(8 * width / 10, 8 * height / 10);
  _renderWindow.reset(new RenderWindow(sf::VideoMode(_windowSize.x, _windowSize.y), "...", sf::Style::Default, settings));
  _renderWindow->setVerticalSyncEnabled(true);
  _eventManager.reset(new WindowEventManager(_renderWindow.get()));

  _eventManager->RegisterHandler(Event::KeyPressed, bind(&Game::OnKeyPressed, this, _1));
  _eventManager->RegisterHandler(Event::KeyReleased, bind(&Game::OnKeyReleased, this, _1));
  _eventManager->RegisterHandler(Event::LostFocus, bind(&Game::OnLostFocus, this, _1));
  _eventManager->RegisterHandler(Event::GainedFocus, bind(&Game::OnGainedFocus, this, _1));
  _eventManager->RegisterHandler(Event::MouseButtonReleased, bind(&Game::OnMouseButtonReleased, this, _1));

  TwInit(TW_OPENGL, NULL);
  TwWindowSize(_windowSize.x, _windowSize.y);
  _twBar = TwNewBar("PangBar");

#ifdef WIN32
  string base("d:/projects/pang/");
#else
  string base("/Users/dooz/projects/pang/");
#endif
  if (!_font.loadFromFile(base + "gfx/04b_03b_.ttf"))
  {
    return false;
  }

  if (!LoadProto((base + "config/game_large.pb").c_str(), &_gameConfig))
    return false;

  if (!_level.Init(_gameConfig))
    return false;

  if (!COORDINATOR.Create())
    return false;

  // create local player
  Vector2f p(0,0);
  p = GetEmptyPos();
  shared_ptr<Entity> e = make_shared<Entity>(_localPlayerId, p);
  _entities[_localPlayerId] = e;

  _level.SetEntity(WorldToTile(e->_pos), e->_id);

  SpawnEnemies();

  return true;
}

//----------------------------------------------------------------------------------
Vector2f Game::GetEmptyPos()
{
  u32 w, h;
  _level.GetSize(&w, &h);
  while (true)
  {
    u32 x = rand() % w;
    u32 y = rand() % h;
    Level::Cell* cell = 0;
    if (_level.GetCell(Tile(x, y), &cell) && cell->terrain == 0)
    {
      return (float)_gridSize * Vector2f(x, y);
    }
  }
  return Vector2f(0,0);
}

//----------------------------------------------------------------------------------
Vector2f Game::GetEmptyPos(const Vector2f& center, float radius)
{
  // find an empty position with LOS to the center
  u32 w, h;
  _level.GetSize(&w, &h);
  Tile tile = WorldToTile(center);
  u32 x0 = tile.x;
  u32 y0 = tile.y;
  while (true)
  {
    u32 x = (u32)((s32)x0 + randf(-radius, radius));
    u32 y = (u32)((s32)y0 + randf(-radius, radius));
    if (_level.IsValidPos(Tile(x, y)) /*&& _level.IsVisible(x0, y0, x, y)*/)
    {
      // Check LOS to the center
      return (float)_gridSize * Vector2f(x, y);
    }
  }
  return Vector2f(0,0);
}


//----------------------------------------------------------------------------------
void Game::SpawnEnemies()
{
  assert(_gameConfig.mobs_per_squad() < 16);
  for (int i = 0; i < _gameConfig.num_squads(); ++i)
  {
    Vector2f squadCenter(GetEmptyPos());
    for (int j = 0; j < _gameConfig.mobs_per_squad(); ++j)
    {
      EntityId idx = ((i+1) << 4) + j;
      shared_ptr<Entity> e = make_shared<Entity>(idx, GetEmptyPos(squadCenter, 4));
      //e->_debug = new PursuitDebugRenderer(e.get());
      e->_debug = new WanderDebugRenderer(e.get());
      e->_squadId = i;
      _entities[idx] = e;

      _level.SetEntity(WorldToTile(e->_pos), e->_id);
    }
  }
}

//----------------------------------------------------------------------------------
void Game::UpdateEnemies()
{
  if (_playerDead || _pausedEnemies)
    return;

  Entity* localPlayer = _entities[_localPlayerId].get();

  Vector2f playerPos(localPlayer->_pos);

  for (auto& kv : _entities)
  {
    Entity* e = kv.second.get();
    if (e->_id == _localPlayerId)
      continue;

//    e->_force = BehaviorPursuit(e, localPlayer);
    e->_force = 0.40f * BehaviorWander(e);
    Level::Cell* cell;
    if (_level.GetCell(WorldToTile(e->_pos), &cell))
    {
      e->_force += 0.60f * BehaviorAvoidWall(e, *cell);
    }

  }

#if 0
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
#endif
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
  float ff = (float)_gridSize;
  float f = ff - 1;
  int x = (int)((pos.x + f) / ff);
  int y = (int)((pos.y + f) / ff);
  return Vector2f(x * ff, y * ff);
}

//----------------------------------------------------------------------------------
Vector2f Game::ClampedDestination(const Vector2f& pos, const Vector2f& dir)
{
  return SnappedPos(pos + (float)_gridSize * dir);
}

//----------------------------------------------------------------------------------
void Game::HandleInput()
{
  if (_playerDead)
    return;

  Entity& e = *_entities[_localPlayerId];

  u8 curLeft = Keyboard::isKeyPressed(Keyboard::Left) || Keyboard::isKeyPressed(Keyboard::A);
  u8 curRight = Keyboard::isKeyPressed(Keyboard::Right) || Keyboard::isKeyPressed(Keyboard::D);

  if (curLeft && !_prevLeft)
  {
    e._rot -= PI/8;
  }
  else if (curRight && !_prevRight)
  {
    e._rot += PI/8;
  }
  else if (Keyboard::isKeyPressed(Keyboard::Up) || Keyboard::isKeyPressed(Keyboard::W))
  {
    e._force += 0.001f * e.Dir();
  }
  else if (Keyboard::isKeyPressed(Keyboard::Down) || Keyboard::isKeyPressed(Keyboard::S))
  {
    e._force -= 0.001f * e.Dir();
  }

  _prevLeft = curLeft;
  _prevRight = curRight;
}

//----------------------------------------------------------------------------------
void Game::DebugDrawEntity()
{
  if (!_selectedEntity)
    return;

  const Entity& e = *_selectedEntity;
  AddMessage(MessageType::Debug, toString("id: %d", e._id));
//  AddMessage(MessageType::Debug, toString("pos: x: %.2f, y: %.2f, rot: %.2f", e._pos.x, e._pos.y, e._rot));
}


//----------------------------------------------------------------------------------
bool Game::SpawnBullet(Entity& e)
{
  #if 0
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
    ActionBullet* b = new ActionBullet(e._id);
    b->pos = pos;
    b->dir = dir;
    _actionQueue.push_back(b);
  }
#endif
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
  switch (event.key.code)
  {
    case Keyboard::Num1: _debugDraw.Toggle(DebugDrawFlags::EnemyInfo); break;
    case Keyboard::Num2: _debugDraw.Toggle(DebugDrawFlags::PlayerInfo); break;
    case Keyboard::Num3: _debugDraw.Toggle(DebugDrawFlags::BehaviorInfo); break;
    case Keyboard::Num4: _debugDraw.Toggle(DebugDrawFlags::PlayerCone); break;
  }

  return true;
}

//----------------------------------------------------------------------------------
void Game::DrawGrid()
{
  float g = (float)_gridSize;
  _levelSprite.setPosition(0, 0);
  _levelSprite.setTexture(_level.GetTexture());
  _levelSprite.setScale(g, g);
  _renderWindow->draw(_levelSprite);

  vector<sf::Vertex> lines;
  Color c(0x80, 0x80, 0x80);

  u32 w, h;
  _level.GetSize(&w, &h);

  // horizontal
  float x = w * g;
  float y = 0;
  for (u32 i = 0; i <= h; ++i)
  {
    lines.push_back(sf::Vertex(Vector2f(0, y), c));
    lines.push_back(sf::Vertex(Vector2f(x, y), c));
    y += g;
  }

  // vertical
  x = 0;
  y = h * g;
  for (u32 i = 0; i <= w; ++i)
  {
    lines.push_back(sf::Vertex(Vector2f(x, 0), c));
    lines.push_back(sf::Vertex(Vector2f(x, y), c));
    x += g;
  }

  _renderWindow->draw(lines.data(), lines.size(), sf::Lines);

}

//----------------------------------------------------------------------------------
void Game::UpdateVisibility()
{
  for (auto& kv : _entities)
  {
    shared_ptr<Entity>& e = kv.second;
    e->_visibleEntities.clear();
    bool localPlayer = e->_id == _localPlayerId;

    float distSq = e->_viewDistance * e->_viewDistance;
    const Vector2f& pos = e->_pos;
    const Vector2f& dir = e->Dir();

    const Tile& t0 = WorldToTile(pos);

    for (auto& innerKv : _entities)
    {
      shared_ptr<Entity>& e2 = innerKv.second;
      if (e->_id == e2->_id || (localPlayer == (e2->_id == _localPlayerId)) )
        continue;

      // first, check distance
      float tmp = DistSq(e->_pos, e2->_pos);
      if (tmp > distSq)
        continue;

      // check angle between direction vector and vector to entity
      Vector2f toEntity(Normalize(e2->_pos - pos));

      // dot(a,b) = cos(theta)
      float angle = acosf(Dot(toEntity, dir));
      if (angle < e->_fov)
      {
        // do a LOS check
        vector<Vector2i> path;
        const Tile& t1 = WorldToTile(e2->_pos);

        if (_level.IsVisible(t0.x, t0.y, t1.x, t1.y))
        {
          e->_visibleEntities.push_back(e2->_id);
          if (!localPlayer)
          {
            // monster has spotted the player, so report it
            COORDINATOR.SendMessage(AiMessage::MakePlayerSpotted(e2->_pos));
            AddMessage(MessageType::Debug, toString("player spotted by: %hd", e->_id));
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------------
void Game::PhysicsUpdate(float delta_ms)
{
  // verlet integration:
  // xi+1 = xi + (xi - xi-1) + a * dt * dt

  float deltaSq = delta_ms * delta_ms;
  float invDelta = 1.0f / delta_ms;

  for (const auto& kv : _entities)
  {
    Entity* e = kv.second.get();
    Vector2f prevPos = e->_pos;
    // F = m/a => a = F/m
    Vector2f damping = -0.001f * e->_vel;
    e->_acc = (e->_force + damping) * e->_invMass;
    e->_force = Vector2f(0,0);
    Vector2f newPos = e->_pos + (e->_pos - e->_prevPos) + e->_acc * deltaSq;

    Level::Cell* cell;
    if (!_level.GetCell(WorldToTile(newPos), &cell) || cell->terrain > 0)
    {
      // penetration, so project the entity backwards
      Vector2f dir = (newPos - prevPos);
      e->_pos -= dir;
    }
    else
    {
      e->_pos = newPos;
    }
    e->_vel = (e->_pos - e->_prevPos) * invDelta;
    e->_prevPos = prevPos;

    if (e->_id != _localPlayerId)
      e->_rot = atan2f(e->_vel.x, -e->_vel.y);
  }
}

//----------------------------------------------------------------------------------
void Game::Update()
{
  _now = microsec_clock::local_time();
  if (_lastUpdate.is_not_a_date_time())
  {
    _lastUpdate = _now;
    return;
  }

  _eventManager->Poll();


  u64 delta_us = (_now - _lastUpdate).total_microseconds();
  _lastUpdate = _now;

  if (_debugDraw.IsSet(DebugDrawFlags::DrawLevel))
    return;

  HandleInput();

  // calc the number of physics ticks to take (the physics run with a fixed time step)
  static const u64 tickFreq = 100;
  static const u64 tick_us = (u64)(1e6f / tickFreq);

  float delta_s = delta_us / 1e6f;
  _tickAcc += delta_us;
  while (_tickAcc > tick_us)
  {
    PhysicsUpdate(tick_us / 1000);
    _tickAcc -= tick_us;
  }


  UpdateVisibility();

  Level::Cell* cell;
  if (_level.GetCell(WorldToTile(_entities[_localPlayerId]->_pos), &cell))
    cell->heat = 255;

  UpdateBullets(delta_s);

  COORDINATOR.Update();
  UpdateEnemies();

}


//----------------------------------------------------------------------------------
void Game::UpdateBullets(float delta_s)
{
  for (auto it = _bullets.begin(); it != _bullets.end();)
  {
    Bullet& b = *it;
    b.pos = b.pos + 100 * delta_s * b.dir;
    if (!_level.IsValidPos(WorldToTile(b.pos)))
    {
      it = _bullets.erase(it);
    }
    else
    {
      bool collision = false;
      // check for player collision
      for (auto j = _entities.begin(); j != _entities.end();)
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

}

//----------------------------------------------------------------------------------
void Game::Render()
{
  _renderWindow->clear();

  TwDraw();

  if (_debugDraw.IsSet(DebugDrawFlags::DrawLevel))
  {
    float s = 4;
    _levelSprite.setPosition(0, 0);
    _levelSprite.setTexture(_level.GetTexture());
    _levelSprite.setScale(s, s);
    _renderWindow->draw(_levelSprite);
  }
  else
  {
    if (!_playerDead)
    {
      Vector2u s = _renderWindow->getSize();
      _view.setCenter(_entities[_localPlayerId]->_pos);
      _view.setRotation(0);
      _view.setSize(VectorCast<float>(s));
      _renderWindow->setView(_view);
    }

//  _level.Diffuse();
//  _level.UpdateTexture();
    DrawGrid();
    DrawEntities();

    DebugDrawEntity();

    if (_playerDead)
    {
      AddMessage(MessageType::Debug, "** GAME OVER **");
    }
  }

  UpdateMessages();
  _renderWindow->display();
}

//----------------------------------------------------------------------------------
void Game::DrawEntities()
{
  float g = (float)_gridSize;
  Vector2f ofs(g/2, g/2);

  for (const auto& kv : _entities)
  {
    const Entity& e = *kv.second;
    VertexArray triangle(sf::Triangles, 3);
    Transform rotation;
    Color col = e._id == _localPlayerId ? Color::Green : Color::Yellow;
    rotation.rotate(180 * e._rot / PI);
    triangle[0].position = ofs + e._pos + rotation.transformPoint(Vector2f(0, -g/2));
    triangle[0].color = Color::Red;
    triangle[1].position = ofs + e._pos + rotation.transformPoint(Vector2f(-5, 0.75f * g/2));
    triangle[1].color = col;
    triangle[2].position = ofs + e._pos + rotation.transformPoint(Vector2f(5, 0.75f * g/2));
    triangle[2].color = col;
    _renderWindow->draw(triangle);

    // draw the visibility cone
    if (_debugDraw.IsSet(DebugDrawFlags::PlayerCone) && e._id == _localPlayerId)
    {
      ArcShape aa(e._pos + ofs, e._viewDistance, e._rot - e._fov, e._rot + e._fov);
      aa.setFillColor(Color(e._visibleEntities.empty() ? 200 : 0, 200, 0, 100));
      _renderWindow->draw(aa);
    }

    if (e._id == _localPlayerId)
    {
      if (_debugDraw.IsSet(DebugDrawFlags::PlayerInfo))
      {
        AddMessage(MessageType::Debug, toString("x: %.2f, y: %.2f", e._pos.x, e._pos.y));
      }
    }
    else
    {

      sf::Text text(toString("%d (%d)", e._id, e._squadId), _font);
      text.setCharacterSize(16);
      text.setPosition(e._pos.x, e._pos.y+10);
      _renderWindow->draw(text);

      if (e._debug && _debugDraw.IsSet(DebugDrawFlags::BehaviorInfo))
      {
        e._debug->Render(_renderWindow.get());
      }

    }

  }

  // draw bullets
  RectangleShape rect;
  rect.setFillColor(Color::Red);
  rect.setSize(Vector2f(6, 6));
  Vector2f bulletOfs(0, 3);

  for (const Bullet& b : _bullets)
  {
    rect.setPosition(b.pos - bulletOfs);
    _renderWindow->draw(rect);
  }

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
    u8 c = (u8)(255 * 0.8f);
    msg.color = Color(c, c, c);
    msg.endTime = microsec_clock::local_time() + seconds(5);
  }
  else if (type == MessageType::Warning)
  {
    msg.color = Color((u8)(0.9f * 255), (u8)(0.9f * 255), 0);
    msg.endTime = microsec_clock::local_time() + seconds(10);
  }
  else
  {
    // error
    msg.color = Color((u8)(0.9f * 255), (u8)(0.2f * 255), 0);
    msg.endTime = microsec_clock::local_time() + seconds(20);
  }

  _messages.push_back(msg);
}

//------------------------------------------------------------------------------
Tile Game::WorldToTile(const Vector2f& p) const
{
  return Tile((u32)(p.x / _gridSize), (u32)(p.y / _gridSize));
}

//------------------------------------------------------------------------------
Vector2f Game::TileToWorld(u32 x, u32 y) const
{
  // returns a point in the center of the tile
  float g = (float)_gridSize;
  return Vector2f(x * g + g / 2, y * g + g / 2);
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
  rect.setSize(Vector2f(16*40, (float)17 * _messages.size()));
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
        msg.color.a = (u8)(255 * left.total_milliseconds() / 1000.0f);
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
