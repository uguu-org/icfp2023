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
      "max_taste=#{max_taste.to_i} ",
      "min_taste=#{min_taste.to_i}\n"
