#ifndef JSON_UTIL_H_
#define JSON_UTIL_H_

#include<stdio.h>

#include<vector>

#include<boost/property_tree/ptree.hpp>
#include<boost/property_tree/json_parser.hpp>

template<typename T>
inline std::vector<T> GetList(const char *key,
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
inline T GetValue(const char *key, const boost::property_tree::ptree &pt)
{
   if( pt.find(key) == pt.not_found() )
   {
      fprintf(stderr, "%s: missing key\n", key);
      return -1;
   }
   return pt.get_child(key).get_value<T>();
}

#endif  // JSON_UTIL_H_
