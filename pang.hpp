#pragma once
#include "types.hpp"
#include "entity.hpp"
#include "level.hpp"
#include "protocol/game.pb.h"

namespace pang
{
  class WindowEventManager;
  struct ActionBase;

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
    void UpdateVisibility();
    u32 ActionScore(const Entity& entity, const AiState& aiState);
    void Render();
    void HandleActions();
    void EraseInProgressMoveActions(EntityId entityId);
    Vector2f GetEmptyPos() const;
    void AddMoveAction(EntityId playerId, const Vector2f& from, const Vector2f& to);
    void DrawGrid();
    void UpdateMessages();
    void Update();

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

    void ReadKeyboard();

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

    deque<ActionBase*> _actionQueue;
    deque<ActionBase*> _inprogressActions;
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
    u32 _debugDraw;
    bool _focus;
    bool _done;
    bool _playerDead;
    bool _pausedEnemies;
    ptime _now;
    ptime _lastUpdate;

    EntityId _localPlayerId;

    u8 _prevLeft, _prevRight;
  };
}