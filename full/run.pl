#!/usr/bin/perl -w

use strict;
use File::Temp qw{tempfile};

# Extract score and stats from SVG file.
sub extract_stats($)
{
   my ($svg) = @_;

   my $score = -1e9;
   my $stats = undef;
   if( open my $handle, "<$svg" )
   {
      while( my $line = <$handle> )
      {
         if( $line =~ />Score = (\S+)</ )
         {
            $score = $1;
            if( defined($stats) )
            {
               close $handle;
               return $score, $stats;
            }
         }
         elsif( $line =~ />(Counters = [^<>]*)</i )
         {
            $stats = $1;
            if( $score > -1e9 )
            {
               close $handle;
               return $score, $stats;
            }
         }
      }
      close $handle;
   }
   return $score, $stats;
}


if( $#ARGV < 0 )
{
   die "$0 {problem IDs}\n";
}

my ($handle, $output_json, $output_svg);
($handle, $output_json) = tempfile("tmp_XXXXXX", DIR=>".", SUFFIX=>".json");
($handle, $output_svg) = tempfile("tmp_XXXXXX", DIR=>".", SUFFIX=>".svg");

foreach my $i (@ARGV)
{
   my $input = "problems/$i.json";
   unless( -s $input )
   {
      unlink $output_json;
      unlink $output_svg;
      die "Missing $input\n";
   }

   # Run solver.
   if( system("./solve $input $output_json $output_svg") != 0 )
   {
      unlink $output_json;
      unlink $output_svg;
      print STDERR "Solver failed on $input\n";
      next;
   }

   # Check if new solution is any better than existing solution, replace
   # existing solution if so.
   my $existing_json = "solutions/$i.json";
   my $existing_svg = "solutions/$i.svg";
   my ($existing_score, $existing_stats) = extract_stats($existing_svg);
   my ($new_score, $new_stats) = extract_stats($output_svg);
   my $info = sprintf '%+.3f, ', $new_score / $existing_score;
   $info .= $new_stats;
   if( $existing_score < $new_score )
   {
      print "Updated $existing_json: $existing_score < $new_score",
            "  $info, +", $new_score - $existing_score, "\n";
      unless( rename $output_json, $existing_json )
      {
         die "Failed to rename $output_json into $existing_json: $!";
      }
      unless( rename $output_svg, $existing_svg )
      {
         die "Failed to rename $output_svg into $existing_svg: $!";
      }
   }
   else
   {
      print "Keeping $existing_json: $existing_score >= $new_score  $info\n";
      unlink $output_json;
      unlink $output_svg;
   }
}
