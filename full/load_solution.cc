#include"load_solution.h"

#include"json_util.h"

void LoadSolutionFromText(const std::string &json_text, Solution *output)
{
   std::istringstream input(json_text);
   boost::property_tree::ptree pt;
   boost::property_tree::read_json(input, pt);

   output->placements.clear();
   output->volumes.clear();

   if( pt.find("placements") == pt.not_found() )
   {
      fputs("Missing placements\n", stderr);
      return;
   }
   for(const auto &i : pt.get_child("placements"))
   {
      output->placements.push_back(XY());
      XY &xy = output->placements.back();
      xy.x = GetValue<double>("x", i.second);
      xy.y = GetValue<double>("y", i.second);
      if( xy.x < 0 || xy.y < 0 )
      {
         output->placements.clear();
         return;
      }
   }

   output->volumes.reserve(output->placements.size());
   if( pt.find("volumes") != pt.not_found() )
   {
      output->volumes = GetList<double>("volumes", pt);
      if( output->volumes.size() > output->placements.size() )
      {
         fprintf(stderr, "Unexpected volume entries (expected %zu, got %zu)\n",
                 output->placements.size(), output->volumes.size());
         output->placements.clear();
         output->volumes.clear();
         return;
      }
   }

   while( output->volumes.size() < output->placements.size() )
      output->volumes.push_back(10);
}
