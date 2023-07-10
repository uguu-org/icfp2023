#ifndef GRID_H_
#define GRID_H_

#include<random>
#include<string>
#include<utility>
#include<vector>
#include"problem.h"

class Grid
{
public:
   // Initialize empty grid.
   explicit Grid(const Problem &problem);

   // Shuffle points_
   void ShufflePoints();

   // Convert (column, row) indices.
   std::pair<int, int> FromXY(const XY &p) const
   {
      return std::make_pair(static_cast<int>((p.x - min_.x) / kCellSize),
                            static_cast<int>((p.y - min_.y) / kCellSize));
   }
   XY ToXY(int column, int row) const
   {
      return {min_.x + column * kCellSize, min_.y + row * kCellSize};
   }

   // Write grid cells.
   void Set(int column, int row, int state)
   {
      grid_[row][column] = static_cast<char>(state);
   }
   void Set(const XY &p, int state)
   {
      Set(static_cast<int>((p.x - min_.x) / kCellSize),
          static_cast<int>((p.y - min_.y) / kCellSize),
          state);
   }

   // Read grid cells.
   int Get(int column, int row) const
   {
      return static_cast<int>(grid_[row][column]);
   }
   int Get(const XY &p) const
   {
      return Get(static_cast<int>((p.x - min_.x) / kCellSize),
                 static_cast<int>((p.y - min_.y) / kCellSize));
   }

   int rows() const { return static_cast<int>(grid_.size()); }
   int columns() const { return static_cast<int>(grid_.front().size()); }
   const std::vector<std::pair<int, int>> &points() const { return points_; }

private:
   static constexpr double kCellSize = 10;

   // Keep track of which cells are occupied.
   std::vector<std::string> grid_;

   // Lower left corner position.
   XY min_;

   // List of shuffled (column, row) values.
   std::vector<std::pair<int, int>> points_;

   // Random state.
   std::random_device rd_;
   std::default_random_engine rng_;
};

#endif  // GRID_H_
