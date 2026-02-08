#ifndef UTILS
#define UTILS

#include <vector>
#include <Arduino_JSON.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <time.h>


tm convert_iso8601_to_tm(String time_in_iso8601);
String to_humanreadable(String time_in_iso8601);

#endif
