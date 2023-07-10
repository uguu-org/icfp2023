#include"problem.h"

#include<math.h>

#include<algorithm>
#include<limits>
#include<vector>

#include"json_util.h"

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

static XY GetCoord(const char *key, const boost::property_tree::ptree &pt)
{
   const std::vector<double> s = GetList<double>(key, pt);
   XY p = {std::numeric_limits<double>::quiet_NaN(), 0.0};
   if( static_cast<int>(s.size()) == 2 )
   {
      p.x = s[0];
      p.y = s[1];
   }
   return p;
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

   stage_bottom_left_ = GetCoord("stage_bottom_left", pt);
   if( isnan(stage_bottom_left_.x) )
   {
      fputs("Bad stage_bottom_left definition\n", stderr);
      return;
   }
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

   for(const auto &i : pt.get_child("pillars"))
   {
      pillars_.push_back(Pillar());
      Pillar &p = pillars_.back();
      p.position = GetCoord("center", i.second);
      if( isnan(p.position.x) )
      {
         fputs("Bad pillar.center definition\n", stderr);
         return;
      }
      p.radius = GetValue<double>("radius", i.second);
      if( p.radius <= 0 )
      {
         fprintf(stderr, "Bad pillar, center=(%g, %g), radius=%g\n",
                 p.position.x, p.position.y, p.radius);
         return;
      }

      // Problem spec didn't say if pillar could be out of bounds,
      // we are not going to verify it here.
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
   static constexpr int kProbeStepSize = 10;

   const double adjusted_width = stage_size_.x - kMargin * 2;
   const double adjusted_height = stage_size_.y - kMargin * 2;
   const int x_resolution =
      std::max(2, static_cast<int>(adjusted_width / kProbeStepSize));
   const int y_resolution =
      std::max(2, static_cast<int>(adjusted_height / kProbeStepSize));

   std::vector<XY> probe;
   probe.reserve(x_resolution * y_resolution);
   for(int y = 0; y < y_resolution; y++)
   {
      const double py = stage_bottom_left_.y +
                        (y * adjusted_height) / (y_resolution - 1);
      for(int x = 0; x < x_resolution; x++)
      {
         const double px = stage_bottom_left_.x +
                           (x * adjusted_width) / (x_resolution - 1);
         probe.push_back(XY());
         probe.back().x = px;
         probe.back().y = py;
      }
   }

   for(Attendee &a : attendees_)
   {
      // Compute influences when all musicians are concentrated at a
      // single point.  This tells us the maximum possible effect that
      // placements could affect a single attendee.
      double max_influence = 0;
      double min_influence = std::numeric_limits<double>::infinity();
      for(const XY &p : probe)
      {
         const double d2 = DistanceSquared(a.position, p);
         if( d2 <= 0 )
            continue;

         if( BlockedByPillar(a.position, p) )
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

bool Problem::BlockedByPillar(const XY &u, const XY &v) const
{
   for(const Pillar &p : pillars_)
   {
      if( IsBlocked(u, v, p.position, p.radius) )
         return true;
   }
   return false;
}
