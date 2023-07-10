#include<stdio.h>
#include"problem.h"
#include"solution.h"

static constexpr double kExpectedScore = 5343;

static constexpr char kProblem[] = R"(
{
   "room_width": 2000.0,
   "room_height": 5000.0,
   "stage_width": 1000.0,
   "stage_height": 200.0,
   "stage_bottom_left": [500.0, 0.0],
   "musicians": [0, 1, 0],
   "attendees": [
      {"x":100.0,  "y":500.0,  "tastes":[1000.0, -1000.0]},
      {"x":200.0,  "y":1000.0, "tastes":[ 200.0,   200.0]},
      {"x":1100.0, "y":800.0,  "tastes":[ 800.0,  1500.0]}
   ]
}
)";

int main(int argc, char **argv)
{
   const Problem problem(kProblem);
   Solution solution;
   solution.placements.resize(3);
   solution.placements[0].x = 590;
   solution.placements[0].y = 10;
   solution.placements[1].x = 1100;
   solution.placements[1].y = 100;
   solution.placements[2].x = 1100;
   solution.placements[2].y = 150;
   solution.score = ComputeScore(problem, solution.placements);

   printf("Expected %g, actual = %g\n", kExpectedScore, solution.score);
   return 0;
}
