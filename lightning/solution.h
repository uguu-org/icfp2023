#ifndef SOLUTION_H_
#define SOLUTION_H_

#include<vector>
#include"problem.h"

struct Solution
{
   std::vector<XY> placements;
   double score;
};

double ComputeScore(const Problem &problem, const std::vector<XY> &placements);
void Solve(const Problem &problem, Solution *solution);

#endif  // SOLUTION_H_
