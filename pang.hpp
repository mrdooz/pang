#pragma once
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
    u32 playerId;
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
    void Render();
    void HandleActions();
    void EraseMoveActions(u32 playerId);
    bool IsValidPos(const Vector2f& p);
    void AddMoveAction(u32 playerId, const Vector2f& from, const Vector2f& to);
    void DrawGrid();
    void UpdateMessages();
    void Update();

    void SpawnEnemies();
    void UpdateEnemies();
    bool SpawnBullet(Entity& e);

    Vector2f ClampedDestination(const Vector2f& pos, const Vector2f& dir);
    Vector2f SnappedPos(const Vector2f& pos);

    bool OnLostFocus(const Event& event);
    bool OnGainedFocus(const Event& event);
    bool OnKeyPressed(const Event& event);
    bool OnKeyReleased(const Event& event);

    void ReadKeyboard();

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
    unordered_map<u32, Entity > _entities;
    unordered_map<u32, Entity > _deadEntites;

    Level _level;
    Sprite _levelSprite;
    View _view;

    vector<Bullet> _bullets;
    vector<Message> _messages;
    Font _font;
    u32 _gridSize;
    u32 _debugDraw;
    bool _focus;
    bool _done;
    bool _playerDead;
    ptime _now;
    ptime _lastUpdate;

    u32 _localPlayerId;

    u8 _prevLeft, _prevRight;
  };
}