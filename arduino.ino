#include <Arduino.h>
#include <Arduino_JSON.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <U8g2lib.h>
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <time.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <vector>

#include "api_helpers.h"
#include "creds.h"
#include "mydrawings.h"

U8G2_SSD1306_128X64_NONAME_F_SW_I2C drawer(U8G2_R0, 14, 12, U8X8_PIN_NONE);

String WEATHER_API_PATH = "http://api.open-meteo.com/v1/forecast?latitude=" latitude "&longitude=" longtitude "&hourly=temperature_2m,precipitation&timezone=Europe\%2FAmsterdam&forecast_days=3";
String TIME_API_PATH = "http://worldtimeapi.org/api/timezone/Europe/Amsterdam";

float EPS = 1e-6;

unsigned long timer_delay = 30000;  // 60 secs
unsigned int weather_delay = 30 * 60000; // 30 minutes 

String starting_time;
String current_time;
time_t system_start_time;
time_t last_weather_update = 0;
JSONVar weather;

unsigned int i = 1;
bool initialized = false;

void setup() {
    Serial.begin(115200);
    Serial.println("Hi Serial");
    drawer.begin();
    drawer.setFont(u8g2_font_7x14B_mf);
    connect_wifi();
    set_starting_time();
    Serial.print("Starting time: ");
    Serial.println(starting_time);
    Serial.println("Timer set to " + String(timer_delay / 1000) + " seconds");
}

void connect_wifi() {
    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    int retries = 100;
    while (WiFi.status() != WL_CONNECTED and retries > 0) {
        delay(500);
        Serial.print(".");
        retries--;
    }

    Serial.println("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
}

int set_starting_time() {
    JSONVar response;
    if (get_time_json(response)) {
        return 1;
    }
    starting_time = (String)response["datetime"];
    time(&system_start_time);
    return 0;
}

int get_weather_json(JSONVar &response) {
    return get_json(WEATHER_API_PATH, response);
}

int get_time_json(JSONVar &response) {
    return get_json(TIME_API_PATH, response);
}

bool has_connection(){
    return WiFi.status() == WL_CONNECTED;
}

void initial_info() {
    String result = "";
    if (has_connection()) {
        print_line(drawer, "Connected to WiFi");
        print_line(drawer, WiFi.localIP().toString());
    }
    else{
        print_line(drawer, "WiFi not connected");
    }

    drawer.sendBuffer();
    initialized = true;
    delay(5000);
    drawer.clearDisplay();
}

void loop() {
    drawer.home();

    if (!initialized) {
        initial_info();
        return;
    }

    parse_and_print();
    heartbeat_to_serial();
    drawer.sendBuffer();

    Serial.println("Delay...");
    delay(timer_delay);
}

void heartbeat_to_serial() {
    Serial.println(i);
    i++;
}

String get_current_time() {
    time_t system_current_time;
    time(&system_current_time);
    int seconds = difftime(system_current_time, system_start_time);

    tm t;
    strptime(starting_time.c_str(), "%Y-%m-%dT%T", &t);
    t.tm_sec += seconds;

    // Normalization
    mktime(&t);

    char buf[sizeof "1970-01-01T00:00"];
    strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M", &t);
    return String(buf);
}

int upper_bound_time(String start_time, std::vector<String> &times) {
    return std::upper_bound(times.begin(), times.end(), start_time) - times.begin();
}

String to_humanreadable(String time_in_iso8601) {
    // "2023-10-08T20:15:21.479523+02:00"
    // "2023-10-08T00:00"
    tm t;
    // strptime(time_in_iso8601.c_str(), "%Y-%m-%dT%H:%M", &t);
    strptime(time_in_iso8601.c_str(), "%Y-%m-%dT%T", &t);

    char buf[sizeof "07:07"];
    strftime(buf, sizeof buf, "%H:%M", &t);
    return String(buf);
}

int find_next(int start, std::vector<float> &rains, bool is_raining) {
    while (start < rains.size()) {
        if ((is_raining && rains[start] < EPS) || (!is_raining && rains[start] > EPS)) {
            return start;
        }
        start++;
    }
    return -1;
}

int update_weather_if_needed() {
    time_t now;
    time(&now);

    bool update_required = false;
    if (last_weather_update == 0) {
        update_required = true;
    }
    else {
        int seconds = difftime(now, last_weather_update);
        update_required = seconds >= weather_delay;
    }

    if (update_required) {
        Serial.println("Weather updated");
        time(&last_weather_update);
        return get_weather_json(weather);
    }
    return 0;
}

int parse_and_print() {
    // Send an HTTP GET request
    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED) {
        if (update_weather_if_needed()) {
            return 1;
        }

        std::vector<String> times = get_strings_from_json(weather["hourly"]["time"]);
        std::vector<float> temps = get_floats_from_json(weather["hourly"]["temperature_2m"]);
        std::vector<float> rains = get_floats_from_json(weather["hourly"]["precipitation"]);

        String current_time = get_current_time();
        int next_time_point = upper_bound_time(current_time, times);
        int cur_time_point = next_time_point > 0 ? next_time_point - 1 : next_time_point;
        print_line(drawer, "Time: " + to_humanreadable(current_time));
        print_line(drawer, "Temperature: " + String(temps[cur_time_point]));

        bool is_raining = rains[cur_time_point] > 0;
        next_time_point = find_next(next_time_point, rains, is_raining);

        if (is_raining) {
            if (next_time_point == -1) {
                print_line(drawer, "Rain forever");
            } else {
                print_line(drawer, "Rain stops: " + to_humanreadable(times[next_time_point]));
            }
        } else {
            if (next_time_point == -1 || next_time_point - cur_time_point > 24) {
                print_line(drawer, "No rains today");
            } else {
                print_line(drawer, "Rain starts: " + to_humanreadable(times[next_time_point]));
            }
        }
    } else {
        print_line(drawer, "WiFi not connected");
        Serial.println("WiFi Disconnected");
    }
    return 0;
}