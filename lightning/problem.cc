#include"problem.h"

#include<math.h>

#include<algorithm>
#include<array>
#include<limits>

#include<boost/property_tree/ptree.hpp>
#include<boost/property_tree/json_parser.hpp>

#ifdef BENCHMARK
   #include<chrono>
   #include<iostream>
#endif

namespace {

static double DistanceSquared(const XY &a, const XY &b)
{
   const double dx = a.x - b.x;
   const double dy = a.y - b.y;
   return dx * dx + dy * dy;
}

template<typename T>
static std::vector<T> GetList(const char *key,
                              const boost::property_tree::ptree &pt)
{
   std::vector<T> values;
   if( pt.find(key) == pt.not_found() )
   {
      fprintf(stderr, "%s: missing key\n", key);
      return values;
   }
   for(const auto &i : pt.get_child(key))
      values.push_back(i.second.get_value<T>());
   return values;
}

template<typename T>
static T GetValue(const char *key, const boost::property_tree::ptree &pt)
{
   if( pt.find(key) == pt.not_found() )
   {
      fprintf(stderr, "%s: missing key\n", key);
      return -1;
   }
   return pt.get_child(key).get_value<T>();
}

}  // namespace

Problem::Problem(const std::string &json_text)
{
   #ifdef BENCHMARK
      const auto start_time = std::chrono::steady_clock::now();
   #endif

   if( json_text.empty() )
      return;

   std::istringstream input(json_text);
   boost::property_tree::ptree pt;
   boost::property_tree::read_json(input, pt);

   room_size_.x = GetValue<double>("room_width", pt);
   room_size_.y = GetValue<double>("room_height", pt);
   stage_size_.x = GetValue<double>("stage_width", pt);
   stage_size_.y = GetValue<double>("stage_height", pt);
   if( room_size_.x <= 0 || room_size_.y <= 0 ||
       stage_size_.x <= 0 || stage_size_.y <= 0 )
   {
      fputs("Bad spec\n", stderr);
      return;
   }

   const std::vector<double> s = GetList<double>("stage_bottom_left", pt);
   if( s.size() != 2 )
   {
      fputs("Bad stage_bottom_left definition\n", stderr);
      return;
   }
   stage_bottom_left_.x = s[0];
   stage_bottom_left_.y = s[1];
   if( stage_bottom_left_.x < 0 ||
       stage_bottom_left_.x + stage_size_.x > room_size_.x ||
       stage_bottom_left_.y < 0 ||
       stage_bottom_left_.y + stage_size_.y > room_size_.y )
   {
      fprintf(stderr,
              "Stage is out of bounds: "
              "room size = (%g,%g), stage = (%g,%g) @ (%g,%g)\n",
              room_size_.x, room_size_.y,
              stage_bottom_left_.x, stage_bottom_left_.y,
              stage_size_.x, stage_size_.y);
      return;
   }

   for(const auto &i : pt.get_child("attendees"))
   {
      attendees_.push_back(Attendee());
      Attendee &a = attendees_.back();
      a.position.x = GetValue<double>("x", i.second);
      a.position.y = GetValue<double>("y", i.second);
      a.tastes = GetList<double>("tastes", i.second);
   }
   if( attendees_.empty() )
   {
      fputs("No attendees\n", stderr);
      return;
   }

   musicians_ = GetList<int>("musicians", pt);
   if( musicians_.empty() )
   {
      fputs("No musicians\n", stderr);
      return;
   }
   int max_instrument = -1;
   for(int i : musicians_)
      max_instrument = std::max(max_instrument, i);
   if( max_instrument < 0 )
   {
      fputs("Invalid musicians\n", stderr);
      musicians_.clear();
      return;
   }
   instruments_.resize(max_instrument + 1);
   std::fill(instruments_.begin(), instruments_.end(), 0);
   for(int i : musicians_)
      instruments_[i]++;

   for(const auto &a : attendees_)
   {
      if( a.tastes.size() != instruments_.size() )
      {
         fputs("Bad attendee\n", stderr);
         musicians_.clear();
         return;
      }
   }

   ComputeInfluences();
   struct SortByInfluenceRange
   {
      inline bool operator()(const Attendee &a, const Attendee &b) const
      {
         return a.max_influence - a.min_influence >
                b.max_influence - b.min_influence;
      }
   };
   std::sort(attendees_.begin(), attendees_.end(), SortByInfluenceRange());

   #ifdef BENCHMARK
      const std::chrono::duration<double> elapsed =
         std::chrono::steady_clock::now() - start_time;
      std::cerr << "Problem load time: " << elapsed.count() << "\n";
   #endif
}

void Problem::ComputeInfluences()
{
   static constexpr double kMargin = 10;

   // 2 3
   // 0 1
   std::array<XY, 4> corner;
   corner[0].x = corner[2].x = stage_bottom_left_.x + kMargin;
   corner[0].y = corner[1].y = stage_bottom_left_.y + kMargin;
   corner[1].x = corner[3].x = stage_bottom_left_.x + stage_size_.x - kMargin;
   corner[2].y = corner[3].y = stage_bottom_left_.y + stage_size_.y - kMargin;

   for(Attendee &a : attendees_)
   {
      // Compute influences when all musicians are concentrated at each corner
      // of the stage.  This tells us the maximum possible effect that
      // placements could affect a single attendee.
      double max_influence = 0;
      double min_influence = std::numeric_limits<double>::infinity();
      for(const XY &p : corner)
      {
         const double d2 = DistanceSquared(a.position, p);
         if( d2 <= 0 )
            continue;

         // Add influences from all musicians.
         double influence = 0;
         for(int i = 0; i < static_cast<int>(instruments_.size()); i++)
            influence += 1e6 * instruments_[i] * fabs(a.tastes[i]) / d2;

         max_influence = std::max(max_influence, influence);
         min_influence = std::min(min_influence, influence);
      }
      if( min_influence < max_influence )
      {
         a.max_influence = max_influence;
         a.min_influence = min_influence;
      }
      else
      {
         a.max_influence = a.min_influence = 0;
      }
   }
}
