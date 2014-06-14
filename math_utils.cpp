#include "math_utils.hpp"

namespace pang
{
  void Line(int x0, int y0, int x1, int y1, vector<Vector2i>* line)
  {
    line->clear();

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

}
