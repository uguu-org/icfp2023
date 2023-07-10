#!/usr/bin/ruby

require 'json'

outer_json = JSON.parse(ARGF.read)
print outer_json['Success']
