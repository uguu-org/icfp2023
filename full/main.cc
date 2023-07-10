#include<stdio.h>

#include<array>
#include<sstream>
#include<string>
#include<vector>

#include"problem.h"
#include"solution.h"
#include"load_solution.h"

namespace {

// If true, enable drawing lines of hate between attendees and the musicians
// they hated.  This greatly increases output file size and honestly is not
// all that helpful, because there is a lot of hate to go around.
static constexpr bool kDrawLinesOfHate = false;

// Load text from file.
static std::string LoadText(const char *filename)
{
   std::string text;
   FILE *infile = fopen(filename, "rb");
   if( infile == nullptr )
   {
      fprintf(stderr, "Failed to open %s\n", filename);
      return text;
   }

   std::array<char, 1024> buffer;
   size_t read_size;
   do
   {
      read_size = fread(buffer.data(), 1, buffer.size(), infile);
      text.append(std::string(buffer.data(), read_size));
   } while( read_size == buffer.size() );
   fclose(infile);
   return text;
}

// Write solution to file.
static void OutputSolution(const Solution &solution, FILE *output)
{
   fputs(R"({"placements":[)", output);

   bool first = true;
   for(const XY &xy : solution.placements)
   {
      if( first )
      {
         first = false;
      }
      else
      {
         fputc(',', output);
      }
      fprintf(output, R"({"x":%f,"y":%f})", xy.x, xy.y);
   }

   fputs(R"(],"volumes":[)", output);
   first = true;
   for(const double volume : solution.volumes)
   {
      if( first )
      {
         first = false;
      }
      else
      {
         fputc(',', output);
      }
      fprintf(output, "%f", volume);
   }

   fputs("]}\n", output);
}

// Generate a random color by hashing an integer.
static unsigned int HashColor(int input)
{
   unsigned int hash = 0;

   // https://en.wikipedia.org/wiki/Jenkins_hash_function
   for(int i = 0; i < 3; i++)
   {
      hash += input & 0xff;
      hash += hash << 10;
      hash ^= hash >> 6;
      input >>= 8;
   }
   hash += hash << 3;
   hash ^= hash >> 11;
   hash += hash << 15;

   hash |= 0x00808080;
   return hash & 0x00ffffff;
}

// Write solution with visualization to file.
static void OutputVisualization(const Problem &problem,
                                const Solution &solution,
                                FILE *output)
{
   // Header.
   fprintf(output, R"(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<svg width="%g" height="%g"
     xmlns="http://www.w3.org/2000/svg"
     xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape"
     xmlns:svg="http://www.w3.org/2000/svg">
)",
           problem.room_size().x, problem.room_size().y);

   // Lines of hate.
   if( kDrawLinesOfHate )
   {
      fputs(R"(<g inkscape:label="Dislikes" inkscape:groupmode="layer" id="dislikes">
)",
            output);
      for(const Problem::Attendee &a : problem.attendees())
      {
         for(int i = 0; i < static_cast<int>(a.tastes.size()); i++)
         {
            if( a.tastes[i] >= 0 )
               continue;
            for(int j = 0; j < static_cast<int>(problem.musicians().size());
                j++)
            {
               if( problem.musicians()[j] != i ||
                   BlockedLineOfSight(problem, solution.placements,
                                      a.position, j) )
               {
                  continue;
               }
               const XY &musician = solution.placements[j];
               fprintf(output, R"(<path style="fill:none;stroke:#%06x;stroke-width:1" d="M %g,%g %g,%g" />
)",
                       HashColor(i),
                       a.position.x, a.position.y,
                       musician.x, musician.y);
            }
         }
      }
      fputs("</g>\n", output);
   }

   // Room.
   fprintf(output, R"(<g inkscape:label="Room" inkscape:groupmode="layer" id="room">
<rect style="fill:none;stroke:#000000" x="0" y="0" width="%g" height="%g" />
<rect style="fill:none;stroke:#ff0000" x="%g" y="%g" width="%g" height="%g" />
)",
           problem.room_size().x, problem.room_size().y,
           problem.stage_bottom_left().x, problem.stage_bottom_left().y,
           problem.stage_size().x, problem.stage_size().y);

   // Pillars.
   for(const Problem::Pillar &p : problem.pillars())
   {
      fprintf(output, R"(<circle style="fill:none;stroke:#ff0000" cx="%g" cy="%g" r="%g" />
)",
              p.position.x, p.position.y, p.radius);
   }
   fputs("</g>\n", output);

   // Attendees.
   fputs(R"(<g inkscape:label="Attendees" inkscape:groupmode="layer" id="attendees">
)",
         output);
   for(const Problem::Attendee &a : problem.attendees())
   {
      int most_hated = -1;
      int min_taste = 0;
      for(int i = 0; i < static_cast<int>(a.tastes.size()); i++)
      {
         if( min_taste > a.tastes[i] )
         {
            min_taste = a.tastes[i];
            most_hated = i;
         }
      }

      if( most_hated < 0 )
      {
         fprintf(output, R"(<circle style="fill:none;stroke:#000000" cx="%g" cy="%g" r="5" />
)",
                 a.position.x, a.position.y);
      }
      else
      {
         fprintf(output, R"(<circle style="fill:#%06x;stroke:#000000" cx="%g" cy="%g" r="5" />
)",
                 HashColor(most_hated),
                 a.position.x, a.position.y);
      }
   }
   fputs("</g>\n", output);

   // Musicians.
   fputs(R"(<g inkscape:label="Musicians" inkscape:groupmode="layer" id="musicians">
)",
         output);
   for(int i = 0; i < static_cast<int>(problem.musicians().size()); i++)
   {
      const XY &m = solution.placements[i];
      const int instrument = problem.musicians()[i];
      fprintf(output, R"(<circle style="fill:#%06x;stroke:#0000ff" cx="%g" cy="%g" r="10" id="m%d_%d" />
)",
              HashColor(instrument), m.x, m.y, i, instrument);
   }
   fputs("</g>\n", output);

   // Debug text.
   std::string counters;
   for(const auto &c : solution.counters)
   {
      if( counters.empty() )
      {
         counters = std::to_string(c);
      }
      else
      {
         counters.push_back(',');
         counters.append(std::to_string(c));
      }
   }

   // Footer.
   static constexpr const char kFontStyle[] =
      "font-family:sans-serif;"
      "font-size:20;"
      "fill:#ff0000;"
      "fill-opacity:1;"
      "stroke:#ffffff;"
      "stroke-opacity:1;"
      "stroke-width:3;"
      "paint-order:stroke fill";
   fprintf(output, R"(<g inkscape:label="Labels" inkscape:groupmode="layer" id="labels">
<text text-anchor="start" x="%g" y="%g" style="%s">Score = %.0f</text>
<text text-anchor="start" x="%g" y="%g" style="%s">Counters = [%s]</text>
<text text-anchor="end" x="%g" y="%g" style="%s">Score = %.0f</text>
<text text-anchor="end" x="%g" y="%g" style="%s">Counters = [%s]</text>
</g>
</svg>
)",
           problem.stage_bottom_left().x,
           problem.stage_bottom_left().y - 2,
           kFontStyle,
           solution.score,
           problem.stage_bottom_left().x,
           problem.stage_bottom_left().y - 22,
           kFontStyle,
           counters.c_str(),
           problem.stage_bottom_left().x + problem.stage_size().x,
           problem.stage_bottom_left().y + problem.stage_size().y + 20,
           kFontStyle,
           solution.score,
           problem.stage_bottom_left().x + problem.stage_size().x,
           problem.stage_bottom_left().y + problem.stage_size().y + 40,
           kFontStyle,
           counters.c_str());
}

}  // namespace

int main(int argc, char **argv)
{
   if( argc != 4 && argc != 5 )
   {
      return fprintf(stderr,
                     "%s {input.json} {output.json} {output.svg} [old.json]\n",
                     *argv);
   }

   Problem problem(LoadText(argv[1]));
   if( !problem.valid() )
   {
      fprintf(stderr, "%s is invalid\n", argv[1]);
      return 1;
   }

   Solution solution;
   if( argc == 5 )
   {
      LoadSolutionFromText(LoadText(argv[4]), &solution);
      if( !UpgradeSolution(problem, &solution) )
         return 1;
   }
   else
   {
      Solve(problem, &solution);
   }

   FILE *outfile = fopen(argv[2], "wb");
   if( outfile == nullptr )
   {
      perror("Error opening output JSON");
   }
   else
   {
      OutputSolution(solution, outfile);
      if( fclose(outfile) != 0 )
         perror("Error writing output JSON");
   }

   outfile = fopen(argv[3], "wb");
   if( outfile == nullptr )
   {
      perror("Error opening output SVG");
   }
   else
   {
      OutputVisualization(problem, solution, outfile);
      if( fclose(outfile) != 0 )
         perror("Error writing output SVG");
   }
   return 0;
}
