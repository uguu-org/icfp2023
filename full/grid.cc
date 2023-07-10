#include"grid.h"

#include<stdlib.h>

#include<algorithm>
#include<cmath>

// Initialize empty grid.
Grid::Grid(const Problem &problem) : rng_(rd_())
{
   min_.x = problem.stage_bottom_left().x + kCellSize;
   min_.y = problem.stage_bottom_left().y + kCellSize;

   const int width = static_cast<int>(problem.stage_size().x / kCellSize) - 1;
   const int height = static_cast<int>(problem.stage_size().y / kCellSize) - 1;
   if( width <= 0 || height <= 0 )
   {
      fprintf(stderr, "Stage too small: (%g, %g)\n",
              problem.stage_size().x, problem.stage_size().y);
      exit(EXIT_FAILURE);
   }

   grid_.reserve(height);
   for(int i = 0; i < height; i++)
      grid_.push_back(std::string(width, '\0'));

   points_.reserve(width * height);
   for(int x = 0; x < width; x++)
   {
      for(int y = 0; y < height; y++)
         points_.push_back(std::make_pair(x, y));
   }
   ShufflePoints();
}

void Grid::ShufflePoints()
{
   std::shuffle(points_.begin(), points_.end(), rng_);
}

void Grid::Reset()
{
   for(std::string &row : grid_)
      std::fill(row.begin(), row.end(), '\0');
}
