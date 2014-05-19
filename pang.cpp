#include "pang.hpp"
#include "window_event_manager.hpp"
#include "action.hpp"
#include "utils.hpp"

using namespace pang;

template <typename T>
bool LoadProto(const char* filename, T* out)
{
  FILE* f = fopen(filename, "rb");
  if (!f)
    return false;

  fseek(f, 0, 2);
  long s = ftell(f);
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
    , _localPlayerId(0)
    , _prevLeft(0)
    , _prevRight(0)
    , _debugDraw(0)
    , _playerDead(false)
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

  string base("/Users/dooz/projects/pang/");
  if (!_font.loadFromFile(base + "gfx/04b_03b_.ttf"))
  {
    return false;
  }

  if (!LoadProto((base + "config/game.pb").c_str(), &_gameConfig))
    return 1;

  _level.Init(_gameConfig.width(), _gameConfig.height());

  Entity& e = _entities[_localPlayerId];
  e._id = _localPlayerId;
  while (true)
  {
    Vector2f pos(rand() % _level._width, rand() % _level._height);
    pos = (float)_gridSize * pos;
    if (IsValidPos(pos))
    {
      e._pos = pos;
      break;
    }
  }

  SpawnEnemies();

  return true;
}

//----------------------------------------------------------------------------------
void Game::SpawnEnemies()
{
  for (u32 i = 0; i < _gameConfig.num_enemies(); ++i)
  {
    u32 idx = _localPlayerId + 1 + i;
    Entity& e = _entities[idx];
    e._id = idx;
    while (true)
    {
      Vector2f pos(rand() % _level._width, rand() % _level._height);
      pos = (float)_gridSize * pos;
      if (IsValidPos(pos))
      {
        e._pos = pos;
        break;
      }
      e._rot = 0;
      e._vel = 0;
    }
  }
}

//----------------------------------------------------------------------------------
void Game::UpdateEnemies()
{
  if (_playerDead)
    return;

  Vector2f playerPos(_entities[_localPlayerId]._pos);

  for (auto& kv : _entities)
  {
    Entity& e = kv.second;
    if (e._id == _localPlayerId)
      continue;

    // turn towards player
    Vector2f dir = (playerPos - e._pos);
    Normalize(dir);

    float a = atan2f(dir.x, dir.y);
    e._rot = a;
    AddMessage(MessageType::Debug, toString("dx: %.2f, dy: %.2f, a: %.2f", dir.x, dir.y, a));
    AddMoveAction(e._id, e._pos, e._pos + (float)_gridSize * dir);

    // dot(a,b) = cos(a/b) * ||a|| * ||b||

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

Vector2f Game::SnappedPos(const Vector2f& pos)
{
  float f = _gridSize;
  int x = (pos.x + f / 2) / f;
  int y = (pos.y + f / 2) / f;
  return Vector2f(x * f, y * f);
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

  Entity& e = _entities[_localPlayerId];

  // 0 = no movement, 1 = x axis, 2 = y axis
  u32 moveAction = 0;

  u8 curLeft = Keyboard::isKeyPressed(Keyboard::Left) || Keyboard::isKeyPressed(Keyboard::A);
  u8 curRight = Keyboard::isKeyPressed(Keyboard::Right) || Keyboard::isKeyPressed(Keyboard::D);

  if (curLeft && !_prevLeft)
  {
    e._rot += PI / 2;
    moveAction = 1;
  }
  else if (curRight && !_prevRight)
  {
    e._rot -= PI / 2;
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
    AddMoveAction(0, e._pos, ClampedDestination(e._pos, e._vel * Vector2f(sinf(e._rot), cosf(e._rot))));
  }

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

  Vector2f dir = Vector2f(sinf(e._rot), cosf(e._rot));
  Vector2f pos = SnappedPos(e._pos) + c + (float)_gridSize * dir;
  if (IsValidPos(pos))
  {
    ActionBullet* b = new ActionBullet();
    b->playerId = e._id;
    b->pos = pos;
    b->dir = dir;
    _actionQueue.push_back(b);
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

  Entity& e = _entities[_localPlayerId];

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
  _levelSprite.setTexture(_level._texture);
  _levelSprite.setScale(_gridSize, _gridSize);
  _renderWindow->draw(_levelSprite);

  vector<sf::Vertex> lines;
  Color c(0x80, 0x80, 0x80);

  // horizontal
  for (u32 i = 0; i <= _level._height; ++i)
  {
    lines.push_back(sf::Vertex(Vector2f(0, i*_gridSize), c));
    lines.push_back(sf::Vertex(Vector2f(_level._width*_gridSize, i*_gridSize), c));
  }

  // vertical
  for (u32 i = 0; i <= _level._width; ++i)
  {
    lines.push_back(sf::Vertex(Vector2f(i*_gridSize, 0), c));
    lines.push_back(sf::Vertex(Vector2f(i*_gridSize, _level._height*_gridSize), c));
  }

  _renderWindow->draw(lines.data(), lines.size(), sf::Lines);

}

//----------------------------------------------------------------------------------
bool Game::IsValidPos(const Vector2f& p)
{
  u8 v = _level.Get(p.x / _gridSize, p.y / _gridSize);
  return v == 0;
}


//----------------------------------------------------------------------------------
void Game::AddMoveAction(u32 playerId, const Vector2f& from, const Vector2f& to)
{
  if (!IsValidPos(from) || !IsValidPos(to))
  {
    _entities[playerId]._vel = 0;
    return;
  }

  // check if a move action to the given location is already in progress
  for (auto it = _inprogressActions.begin(); it != _inprogressActions.end(); ++it)
  {
    ActionBase* a = *it;
    if (a->type == ActionType::Move && a->playerId == playerId)
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
    if (a->type == ActionType::Move && a->playerId == playerId)
    {
      // if a move action for the current player exists in the queue, replace it
      m = static_cast<ActionMove*>(a);
      break;
    }
  }

  // Create the new action if needed
  if (!m)
  {
    m = new ActionMove();
    m->playerId = playerId;
    _actionQueue.push_back(m);
  }

  m->from = from;
  m->to = to;
  m->startTime = _now;
  m->endTime = _now + milliseconds(500);
}

//----------------------------------------------------------------------------------
void Game::EraseMoveActions(u32 playerId)
{
  for (auto it = _inprogressActions.begin(); it != _inprogressActions.end();)
  {
    ActionBase* a = *it;
    if (a->type == ActionType::Move && a->playerId == playerId)
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
    if (!IsValidPos(b.pos))
    {
      it = _bullets.erase(it);
    }
    else
    {
      bool collision = false;
      // check for player collision
      for (auto it = _entities.begin(); it != _entities.end(); )
      {
        Entity& e = it->second;
        if (e._id != b.playerId && SnappedPos(e._pos) == SnappedPos(b.pos))
        {
          collision = true;
          if (e._id == _localPlayerId)
            _playerDead = true;
          _deadEntites[e._id] = e;
          _entities.erase(it);
          break;
        }
        else
        {
          ++it;
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
    u32 playerId = action->playerId;

    bool instantAction = false;

    if (action->type == ActionType::Move)
    {
      // only allow a single in progress move action, so erase any in progress for
      // the current entity
      EraseMoveActions(playerId);
    }
    else if (action->type == ActionType::Bullet)
    {
      ActionBullet* a = static_cast<ActionBullet*>(action);
      Bullet b;
      b.pos = a->pos;
      b.dir = a->dir;
      b.playerId = a->playerId;
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
      Entity& e = _entities[m->playerId];
      // interpolate the player position
      float delta = (float)(_now - m->startTime).total_milliseconds() / (m->endTime - m->startTime).total_milliseconds();
      e._pos = m->from + delta * (m->to - m->from);

      if (_now >= m->endTime)
      {
        deleteAction = true;
        e._vel = 0;
        e._pos = m->to;
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
    _view.setCenter(_entities[_localPlayerId]._pos);
    _view.setRotation(0);
    _view.setSize(s.x, s.y);
    _renderWindow->setView(_view);
  }

  DrawGrid();

  Vector2f c(_gridSize/2, _gridSize/2);

  for (const auto& kv : _entities)
  {
    const Entity& e = kv.second;
    VertexArray triangle(sf::Triangles, 3);
    Transform rotation;
    Color col = e._id == _localPlayerId ? Color::Green : Color::Yellow;
    rotation.rotate(-e._rot * 180 / PI);
    triangle[0].position = c + e._pos + rotation.transformPoint(Vector2f(0, 20));
    triangle[0].color = col;
    triangle[1].position = c + e._pos + rotation.transformPoint(Vector2f(-5, 0));
    triangle[1].color = col;
    triangle[2].position = c + e._pos + rotation.transformPoint(Vector2f(5, 0));
    triangle[2].color = col;
    _renderWindow->draw(triangle);

    if (e._id == _localPlayerId)
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

    Vector2f localPos(_entities[_localPlayerId]._pos);

    for (const auto& kv : _entities)
    {
      const Entity& e = kv.second;
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
void Game::UpdateMessages()
{
  ptime now = microsec_clock::local_time();

  Vector2f center = _renderWindow->getView().getCenter();
  float x = center.x + 300;
  float y = center.y + 0;

  Text text;
  text.setFont(_font);
  text.setCharacterSize(16);

  for (auto it = _messages.begin(); it != _messages.end(); )
  {
    Message& msg = *it;
    if (msg.endTime.is_not_a_date_time() || msg.endTime > now)
    {
      y += 15;
      // blend out alpha over the last second
      time_duration left = msg.endTime - now;
      if (left < seconds(1))
      {
        msg.color.a = 255 * left.total_milliseconds() / 1000.0f;
      }

      text.setPosition(x, y);
      text.setString(msg.str);
      _renderWindow->draw(text);
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
