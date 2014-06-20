#pragma once
#include "level.hpp"


namespace pang
{
  struct Entity;

  struct Entity;

  struct DebugRenderer
  {
    DebugRenderer(const Entity* entity) : _entity(entity) {}
    virtual ~DebugRenderer() {};
    const Entity* _entity;
    virtual void Render(RenderWindow* window) = 0;
  };

  struct PursuitDebugRenderer : public DebugRenderer
  {
    PursuitDebugRenderer(const Entity* entity) : DebugRenderer(entity) {}
    virtual void Render(RenderWindow* window);
    Vector2f _lookAhead;
  };

  struct WanderDebugRenderer : public DebugRenderer
  {
    WanderDebugRenderer(const Entity* entity) : DebugRenderer(entity) {}
    virtual void Render(RenderWindow* window);
  };

  enum class Behavior
  {
    Seek,
    Arrive,
    Pursuit,
    Wander,
    AvoidWall,
    BehaviorCount,
  };

  typedef array<float, (u32)Behavior::BehaviorCount> BehaviorProfile;

  Vector2f ApplyBehaviorProfile(const Entity* e, const BehaviorProfile& p);

  Vector2f BehaviorSeek(const Entity* e, const Vector2f& dest);
  Vector2f BehaviorArrive(const Entity* e, const Vector2f& dest);
  Vector2f BehaviorPursuit(const Entity* e, const Entity* target);

  Vector2f BehaviorWander(const Entity* e);
  Vector2f BehaviorAvoidWall(const Entity* e, const Level::Cell& cell);

  enum class AiMessageType
  {
    PlayerSpotted,
  };


  struct AiMessage
  {
    AiMessage() {}
    AiMessageType type;
    union
    {
      struct
      {
        Vector2f pos;

      } playerSpotted;
    };

    static AiMessage MakePlayerSpotted(const Vector2f& pos)
    {
      AiMessage msg;
      msg.type = AiMessageType::PlayerSpotted;
      msg.playerSpotted.pos = pos;
      return msg;
    }
  };

  struct Coordinator
  {
    static bool Create();
    static bool Destroy();
    static Coordinator& Instance();

    void SendMessage(const AiMessage& msg);
    void Update();

  private:
    static Coordinator* _instance;

    deque<AiMessage> _messageQueue;
  };

  #define COORDINATOR Coordinator::Instance()


}