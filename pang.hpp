#pragma once
#include "types.hpp"
#include "entity.hpp"
#include "level.hpp"
#include "protocol/game.pb.h"
#include "flags.hpp"

namespace pang
{
  class WindowEventManager;

  enum class MessageType
  {
    Debug,
    Info,
    Warning,
    Error,
  };


  struct Bullet
  {
    u16 entityId;
    Vector2f dir;
    Vector2f pos;
  };


  class Game
  {
  public:
    Game();
    bool Init();
    bool Run();
    bool Close();

    void AddMessage(MessageType type, const string& str);

  private:
    bool IsVisible(u32 x0, u32 y0, u32 x1, u32 y1);
    void UpdateVisibility();
    void Render();
    Vector2f GetEmptyPos() const;
    Vector2f GetEmptyPos(const Vector2f& center, float radius);
    void DrawGrid();
    void DrawEntities();
    void DrawBullets();
    void UpdateBullets(float delta_s);
    void UpdateMessages();
    void Update();

    void PhysicsUpdate(float delta_ms);

    void SpawnEnemies();
    void UpdateEnemies();
    bool SpawnBullet(Entity& e);
    void DebugDrawEntity();

    Vector2f ClampedDestination(const Vector2f& pos, const Vector2f& dir);
    Vector2f SnappedPos(const Vector2f& pos);

    bool OnLostFocus(const Event& event);
    bool OnGainedFocus(const Event& event);
    bool OnKeyPressed(const Event& event);
    bool OnKeyReleased(const Event& event);

    bool OnMouseButtonReleased(const Event& event);

    void HandleInput();

    Tile WorldToTile(const Vector2f& p) const;
    Vector2f TileToWorld(u32 x, u32 y) const;

    struct Message
    {
      string str;
      ptime endTime;
      Color color;
    };

    pang::config::Game _gameConfig;

    unique_ptr<RenderWindow> _renderWindow;
    unique_ptr<WindowEventManager> _eventManager;

    unordered_map<EntityId, shared_ptr<Entity> > _entities;
    unordered_map<EntityId, shared_ptr<Entity> > _deadEntites;

    Level _level;
    Sprite _levelSprite;
    View _view;

    shared_ptr<Entity> _selectedEntity;

    vector<Bullet> _bullets;
    vector<Message> _messages;
    Font _font;
    u32 _gridSize;
    struct DebugDrawFlags {
      enum Enum { EnemyInfo = 0x1, PlayerInfo = 0x2, BehaviorInfo = 0x4, PlayerCone = 0x8 };
      struct Bits { u32 enemyInfo : 1; u32 playerInfo : 1; u32 behaviorInfo : 1; u32 playerCone : 1; };
    };
    Flags<DebugDrawFlags> _debugDraw;
    bool _focus;
    bool _done;
    bool _playerDead;
    bool _pausedEnemies;
    ptime _now;
    ptime _lastUpdate;

    EntityId _localPlayerId;

    u8 _prevLeft, _prevRight;
    u64 _tickAcc;
    Vector2i _windowSize;
    TwBar* _twBar;
  };
}