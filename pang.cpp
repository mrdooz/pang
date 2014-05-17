#include "pang.hpp"
#include "window_event_manager.hpp"
#include "action.hpp"
#include "utils.hpp"

using namespace pang;

namespace
{
  Vector2f ClampVector(const Vector2f& v)
  {
    return Vector2f(floor(v.x), floor(v.y));
  }

  Vector2f ClampVector(const Vector2f& v, float f)
  {
    int x = v.x / f;
    int y = v.y / f;
    return Vector2f(x * f, y * f);
  }
}


//----------------------------------------------------------------------------------
Game::Game()
    : _gridSize(25)
    , _focus(true)
    , _done(false)
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

//----------------------------------------------------------------------------------
bool Game::OnKeyPressed(const Event& event)
{
  Keyboard::Key key = event.key.code;

  Entity& e = _entities[0];

  float s = 25;

  switch (key)
  {
    case Keyboard::Left:
      AddMoveAction(0, e._pos, ClampVector(e._pos + Vector2f(-s, 0), s));
      break;

    case Keyboard::Right:
      AddMoveAction(0, e._pos, ClampVector(e._pos + Vector2f(+s, 0), s));
      break;

    case Keyboard::Up:
      AddMoveAction(0, e._pos, ClampVector(e._pos + Vector2f(0, -s), s));
      break;

    case Keyboard::Down:
      AddMoveAction(0, e._pos, ClampVector(e._pos + Vector2f(0, +s), s));
      break;

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

void Game::DrawGrid()
{
  vector<sf::Vertex> lines;

  // horizontal
  for (u32 i = 0; i < 11; ++i)
  {
    lines.push_back(Vector2f(0, i*_gridSize));
    lines.push_back(Vector2f(10 * _gridSize, i*_gridSize));
  }

  // vertical
  for (u32 i = 0; i < 11; ++i)
  {
    lines.push_back(Vector2f(i*_gridSize, 0));
    lines.push_back(Vector2f(i*_gridSize, 10 * _gridSize));
  }

  _renderWindow->draw(lines.data(), lines.size(), sf::Lines);

}

//----------------------------------------------------------------------------------
bool Game::IsValidPos(const Vector2f& p)
{
  if (p.x < 0 || p.x > 1000)
    return false;

  if (p.y < 0 || p.y > 1000)
    return false;

  return true;
}


//----------------------------------------------------------------------------------
void Game::AddMoveAction(u32 playerId, const Vector2f& from, const Vector2f& to)
{
  if (!IsValidPos(from) || !IsValidPos(to))
    return;

  // check if a move action to the given location is already in progress
  for (auto it = _inprogressActions.begin(); it != _inprogressActions.end(); ++it)
  {
    ActionBase* a = *it;
    if (a->type == ActionType::Move && a->playerId == playerId)
    {
      ActionMove* m = static_cast<ActionMove*>(a);
      if (ClampVector(m->to) == ClampVector(to))
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
  m->endTime = _now + seconds(1);
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
void Game::HandleActions()
{
  // Move actions from the queue to the in progress queue
  for (ActionBase* action : _actionQueue)
  {
    u32 playerId = action->playerId;

    if (action->type == ActionType::Move)
    {
      // only allow a single in progress move action, so erase any in progress for
      // the current entity
      EraseMoveActions(playerId);
    }

    _inprogressActions.push_back(action);
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
//    rotation.rotate(lp._rotation);
    triangle[0].position = c + rotation.transformPoint(e._pos + Vector2f(0, 20));
    triangle[1].position = c + rotation.transformPoint(e._pos + Vector2f(-10, 0));
    triangle[2].position = c + rotation.transformPoint(e._pos + Vector2f(10, 0));

    AddMessage(MessageType::Debug, toString("x: %f, y: %f", e._pos.x, e._pos.y));

    _renderWindow->draw(triangle);
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
    u8 c = 255 * 0.8f;
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