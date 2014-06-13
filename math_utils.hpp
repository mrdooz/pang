#pragma once
namespace pang
{
  inline float Dot(const Vector2f& a, const Vector2f& b)
  {
    return a.x * b.x + a.y * b.y;
  }

  template <typename T>
  float Length(const sf::Vector2<T>& a)
  {
    return sqrtf(a.x*a.x + a.y*a.y);
  }

  template <typename T>
  float Dist(const sf::Vector2<T>& a, const sf::Vector2<T>& b)
  {
    float dx = a.x - b.x;
    float dy = a.y - b.y;

    return sqrtf(dx*dx+dy*dy);
  }

  template <typename T>
  float DistSq(const sf::Vector2<T>& a, const sf::Vector2<T>& b)
  {
    float dx = a.x - b.x;
    float dy = a.y - b.y;

    return dx*dx+dy*dy;
  }

  template <typename T>
  sf::Vector2<T> Normalize(const sf::Vector2<T>& a)
  {
    float len = Length(a);
    return len > 0 ? 1/len * a : sf::Vector2<T>(0, 0);
  }

  template <typename T>
  void Normalize(sf::Vector2<T>& a)
  {
    float len = Length(a);
    if (len != 0)
    {
      T recip = 1/len;
      a.x *= recip;
      a.y *= recip;
    }
  }

}