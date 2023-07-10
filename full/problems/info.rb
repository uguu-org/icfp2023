#!/usr/bin/ruby

require 'json'

data = JSON.parse(ARGF.read)
max_instrument = -1

max_taste = data['attendees'].map{|a| a['tastes'].max}.max
min_taste = data['attendees'].map{|a| a['tastes'].min}.min

print "room=#{data['room_width'].to_i}*#{data['room_height'].to_i} ",
      "stage=#{data['stage_width'].to_i}*#{data['stage_height'].to_i} ",
      "musicians=#{data['musicians'].size} ",
      "instruments=#{data['musicians'].max + 1} ",
      "attendees=#{data['attendees'].size} ",
      "pillars=#{data['pillars'].size} ",
      "max_taste=#{max_taste.to_i} ",
      "min_taste=#{min_taste.to_i}\n"

# Confirm that pillars do not intersect with stage.
#
# This could have false positives because pillars are circular rather than
# rectangular, but fortunately none of the problems had pillars that close
# to the stage.
data['pillars'].each{|p|
   stage_x0 = data['stage_bottom_left'][0] - p['radius']
   stage_y0 = data['stage_bottom_left'][1] - p['radius']
   stage_x1 = data['stage_bottom_left'][0] + data['stage_width'] + p['radius']
   stage_y1 = data['stage_bottom_left'][1] + data['stage_height'] + p['radius']

   px = p['center'][0]
   py = p['center'][1]

   if stage_x0 <= px and px <= stage_x1 and stage_y0 <= py and py <= stage_y1
      print "Dangerous pillar #{p}\n"
   end
}

# Check coverage of each instrument.
instruments = {}
data['musicians'].each{|m| instruments[m] = 1}
(instruments.keys.min .. instruments.keys.max).each{|i|
   unless instruments[i]
      print "No musicians for instrument #{i}\n"
   end
}
