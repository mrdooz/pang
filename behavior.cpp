#include "behavior.hpp"
#include "entity.hpp"

using namespace bristol;

namespace pang
{
  BehaviorSettings::BehaviorSettings()
  : wallDist(1.5f)
  , wallForce(3.0f)
  {
  }

  BehaviorSettings g_behaviorSettings;

  namespace
  {
    const float MAX_SPEED = 1.0f;

    const float ANGLE_JITTER = 0.01f;


    struct WanderState
    {
      WanderState() { memset(this, 0, sizeof(WanderState)); }
      float _circleOffset;
      float _circleRadius;
      float _curAngle;
      u32 _ticks;
      int _dir;
    };


    unordered_map<EntityId, WanderState> s_wanderState;

    //----------------------------------------------------------------------------------
    Vector2f CircleCenter(const Entity* e, const WanderState& s)
    {
      return e->_pos + s._circleOffset * Normalize(e->_vel);
    }

  }

  //----------------------------------------------------------------------------------
  void PursuitDebugRenderer::Render(RenderWindow* window)
  {
    LineShape ll(_entity->_pos, _lookAhead);
    ll.setFillColor(Color::Green);
    window->draw(ll);
  }

  //----------------------------------------------------------------------------------
  void WanderDebugRenderer::Render(RenderWindow* window)
  {
    auto it = s_wanderState.find(_entity->_id);
    if (it == s_wanderState.end())
      return;

    WanderState& s = it->second;
    Vector2f center = CircleCenter(_entity, s);
    LineShape ll(_entity->_pos, center);
    ll.setFillColor(Color::Green);
    window->draw(ll);

    Vector2f pt = center + s._circleRadius * Vector2f(cosf(s._curAngle), sinf(s._curAngle));
    LineShape ll2(_entity->_pos, pt);
    ll2.setFillColor(Color::Yellow);
    window->draw(ll2);

  }


  //----------------------------------------------------------------------------------
  Vector2f BehaviorSeek(const Entity* e, const Vector2f& dest)
  {
    // returns a force towards the dest
    Vector2f desiredVel = MAX_SPEED * Normalize(dest - e->_pos);
    return desiredVel - e->_vel;
  }

//----------------------------------------------------------------------------------
  Vector2f BehaviorArrive(const Entity* e, const Vector2f& dest)
  {
    float dist = Length(dest - e->_pos);
    if (dist == 0)
      return Vector2f(0,0);

    Vector2f desiredVel = MAX_SPEED * Normalize(dist * (dest - e->_pos));
    return desiredVel - e->_vel;
  }

  //----------------------------------------------------------------------------------
  Vector2f BehaviorPursuit(const Entity* e, const Entity* target)
  {
    Vector2f toTarget(target->_pos - e->_pos);

    float s = Length(toTarget) / (Length(e->_vel) + Length(target->_vel));
    Vector2f v = target->_pos + s * target->_vel;
    if (e->_debug)
    {
      // meh, is there a better way to do this?
      PursuitDebugRenderer* p = static_cast<PursuitDebugRenderer*>(e->_debug);
      p->_lookAhead = v;
    }
    return BehaviorSeek(e, v);
  }

  //----------------------------------------------------------------------------------
  Vector2f BehaviorWander(const Entity* e)
  {
    WanderState& s = s_wanderState[e->_id];
    if (s._circleOffset == 0)
    {
      // init
      s._circleOffset = 10;
      s._circleRadius = 10;
      s._curAngle = randf(-PI, PI);
      s._ticks = randf(500, 1000);
      s._dir = rand() % 2 ? -1 : 1;
    }

    // project wander circle in front of entity
    Vector2f center = CircleCenter(e, s);

    // update wander angle
    s._curAngle += s._dir * randf(0.0f, ANGLE_JITTER);
    if (--s._ticks == 0)
    {
      // choose new direction
      s._ticks = randf(500, 1000);
      s._dir = rand() % 2 ? -1 : 1;
    }

    // point on circle
    Vector2f pt = center + s._circleRadius * Vector2f(cosf(s._curAngle), sinf(s._curAngle));

    // force towards point
    return Normalize(pt - e->_pos);
  }

  //----------------------------------------------------------------------------------
  Vector2f BehaviorAvoidWall(const Entity* e, const Level::Cell& cell)
  {
    Vector2f res(0,0);

    float dist = g_behaviorSettings.wallDist;
    float scale = g_behaviorSettings.wallForce;
//    const auto& fnScale = [](float s) { return s > 5 ? 0 : max(0.0f, min(1.0f, expf(5 - s / 5.0f))); };
    const auto& fnScale = [=](float s) { return s > dist ? 0 : lerp(0.0f, scale, s / dist); };

    res += fnScale(cell.GetWallDistN()) * Vector2f(0,+1);
    res += fnScale(cell.GetWallDistS()) * Vector2f(0,-1);
    res += fnScale(cell.GetWallDistE()) * Vector2f(-1,0);
    res += fnScale(cell.GetWallDistW()) * Vector2f(+1,0);

    return res;
  }

  //----------------------------------------------------------------------------------
  Vector2f ApplyBehaviorProfile(const Entity* e, const BehaviorProfile& p)
  {
    Vector2f res(0,0);

    return res;
  }


  //----------------------------------------------------------------------------------
  Coordinator* Coordinator::_instance;

  //----------------------------------------------------------------------------------
  bool Coordinator::Create()
  {
    assert(!_instance);
    _instance = new Coordinator();
    return true;
  }

  //----------------------------------------------------------------------------------
  bool Coordinator::Destroy()
  {
    assert(_instance);
    delete exch_null(_instance);
    return true;
  }


  //----------------------------------------------------------------------------------
  Coordinator& Coordinator::Instance()
  {
    return *_instance;
  }

  //----------------------------------------------------------------------------------
  void Coordinator::SendMessage(const AiMessage& msg)
  {
    _messageQueue.push_back(msg);
  }

  //----------------------------------------------------------------------------------
  void Coordinator::Update()
  {
    for (const AiMessage& msg : _messageQueue)
    {
      if (msg.type == AiMessageType::PlayerSpotted)
      {

      }
    }

    _messageQueue.clear();

  }




}
