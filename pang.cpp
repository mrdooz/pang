#include "pang.hpp"
#include "window_event_manager.hpp"
#include "action.hpp"
#include "utils.hpp"

using namespace pang;

//----------------------------------------------------------------------------------
void Level::Init(u32 width, u32 height)
{
  _width = width;
  _height = height;

  _data.resize(_width * _height, 0);

  // create some random blocks :)
  for (u32 i = 0; i < 100; ++i)
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

//----------------------------------------------------------------------------------
Game::Game()
    : _gridSize(25)
    , _focus(true)
    , _done(false)
    , _prevLeft(0)
    , _prevRight(0)
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

  _level.Init(100, 100);

  Entity& e = _entities[0];
  e._pos = Vector2f(100, 100);
  e._id = 0;

  return true;
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
  // 0 = no movement, 1 = x axis, 2 = y axis
  u32 moveAction = 0;
  Entity& e = _entities[0];

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
bool Game::OnKeyPressed(const Event& event)
{
  Keyboard::Key key = event.key.code;
  Entity& e = _entities[0];
  Vector2f c(_gridSize/2, _gridSize/2);

  switch (key)
  {
    case Keyboard::Space:
    {
      Vector2f dir = Vector2f(sinf(e._rot), cosf(e._rot));
      Vector2f pos = e._pos + c + (float)_gridSize * dir;
      if (IsValidPos(pos))
      {
        ActionBullet* b = new ActionBullet();
        b->playerId = e._id;
        b->pos = pos;
        b->dir = dir;
        _actionQueue.push_back(b);
      }
/*
      Bullet b;
      b.dir = Vector2f(sinf(e._rot), cosf(e._rot));
      b.pos = e._pos + c + (float)_gridSize * b.dir;
      b.playerId = e._id;
      _bullets.push_back(b);
*/
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

  Vector2u s = _renderWindow->getSize();

  vector<sf::Vertex> lines;
  Color c(0x80, 0x80, 0x80);

  // horizontal
  for (u32 i = 0; i <= s.y; i += _gridSize)
  {
    lines.push_back(sf::Vertex(Vector2f(0, i), c));
    lines.push_back(sf::Vertex(Vector2f(s.x, i), c));
  }

  // vertical
  for (u32 i = 0; i <= s.x; i += _gridSize)
  {
    lines.push_back(sf::Vertex(Vector2f(i, 0), c));
    lines.push_back(sf::Vertex(Vector2f(i, s.y), c));
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
      for (auto& kv : _entities)
      {
        Entity& e = kv.second;
        if (SnappedPos(e._pos) == SnappedPos(b.pos))
        {
          collision = true;
          e._alive = false;
          break;
        }
      }

      it = collision ? _bullets.erase(it) : ++it;
    }
  }

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
  Vector2f c(_gridSize/2, _gridSize/2);

  for (const auto& kv : _entities)
  {
    const Entity& e = kv.second;

    VertexArray triangle(sf::Triangles, 3);
    Transform rotation;
    rotation.rotate(-e._rot * 180 / PI);
    triangle[0].position = c + e._pos + rotation.transformPoint(Vector2f(0, 20));
    triangle[1].position = c + e._pos + rotation.transformPoint(Vector2f(-5, 0));
    triangle[2].position = c + e._pos + rotation.transformPoint(Vector2f(5, 0));

    AddMessage(MessageType::Debug, toString("x: %.2f, y: %.2f", e._pos.x, e._pos.y));

    _renderWindow->draw(triangle);
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
}

//----------------------------------------------------------------------------------
bool Game::Run()
{
  while (_renderWindow->isOpen() && !_done)
  {
    _now = microsec_clock::local_time();
    _renderWindow->clear();
    _eventManager->Poll();
    Update();
    ReadKeyboard();
    DrawGrid();
    Render();
    UpdateMessages();
    HandleActions();

    _renderWindow->display();
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

  float x = 300;
  float y = 0;

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
