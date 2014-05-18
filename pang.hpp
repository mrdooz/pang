#pragma once
#include "entity.hpp"

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

  struct Level
  {
    Level() : _width(0), _height(0) {}

    void Init(u32 width, u32 height);
    void Set(u32 x, u32 y, u8 v);
    u8 Get(u32 x, u32 y);
    void CreateTexture();

    bool Idx(u32 x, u32 y, u32* idx);

    Texture _texture;
    u32 _width, _height;
    vector<u8> _data;
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

    unique_ptr<RenderWindow> _renderWindow;
    unique_ptr<WindowEventManager> _eventManager;

    deque<ActionBase*> _actionQueue;
    deque<ActionBase*> _inprogressActions;
    unordered_map<u32, Entity > _entities;

    Level _level;
    Sprite _levelSprite;

    vector<Message> _messages;
    Font _font;
    u32 _gridSize;
    bool _focus;
    bool _done;
    ptime _now;

    u8 _prevLeft, _prevRight;
  };
}