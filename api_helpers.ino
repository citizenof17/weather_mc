#include "api_helpers.h"

std::vector<String> get_strings_from_json(JSONVar array)
{
    std::vector<String> res;

    for (int i = 0; i < array.length(); i++)
    {
        res.push_back((String)array[i]);
    }
    return res;
}

std::vector<int> get_ints_from_json(JSONVar array)
{
    std::vector<int> res;

    for (int i = 0; i < array.length(); i++)
    {
        res.push_back((int)array[i]);
    }
    return res;
}

std::vector<float> get_floats_from_json(JSONVar array)
{
    std::vector<float> res;

    for (int i = 0; i < array.length(); i++)
    {
        res.push_back((double)array[i]);
    }
    return res;
}

int get_json(String url, JSONVar &json)
{
    WiFiClient client;
    HTTPClient http;

    http.begin(client, url);

    int response_code = http.GET();

    String json_buffer = "{}";

    if (response_code > 0) {
        json_buffer = http.getString();
    }
    else {
        Serial.print("Error occured, code: ");
        Serial.println(response_code);
    }
    http.end();

    if (response_code != 200)
    {
        return 1;
    }

    // Serial.println(json_buffer);
    json = JSON.parse(json_buffer);

    if (JSON.typeof(json) == "undefined")
    {
        Serial.println("Parsing input failed!");
        return 1;
    }
    return 0;
}