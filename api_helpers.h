#ifndef API_HELPERS
#define API_HELPERS

#include <vector>
#include <Arduino_JSON.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>

std::vector<String> get_strings_from_json(JSONVar array);
std::vector<int> get_ints_from_json(JSONVar array);
std::vector<float> get_floats_from_json(JSONVar array);
int get_json(String url, JSONVar &json);

#endif