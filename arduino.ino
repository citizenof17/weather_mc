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
#include "utils.h"

U8G2_SSD1306_128X64_NONAME_F_SW_I2C drawer(U8G2_R0, 14, 12, U8X8_PIN_NONE);

// NOTE: Some parameters must be provided via creds.h
String WEATHER_API_PATH = "http://api.open-meteo.com/v1/forecast?latitude=" latitude "&longitude=" longtitude "&hourly=temperature_2m,precipitation&timezone=Europe\%2FAmsterdam&forecast_days=3";
String BUSSES_API_PATH = "https://v0.ovapi.nl/tpc/" bustpc;
String TIME_API_PATH = "http://worldtimeapi.org/api/timezone/Europe/Amsterdam";
// String TIME_API_PATH = "https://timeapi.io/api/time/current/coordinate?latitude=" latitude "&longitude=" longtitude;

float EPS = 1e-6;
int INF = 1e9;

// unsigned long TIMER_DELAY = 60000;  // 60 secs
unsigned long TIMER_DELAY = 30000;  // 30 secs
unsigned int WEATHER_API_DELAY = 30 * 60; // 30 minutes 
unsigned int BUS_API_DELAY = 1 * 59; // almost 1 minute
unsigned int BUSSES_TO_SHOW = 3;

// String starting_time = "2020-01-01T00:00:00.0+02:00";
String starting_time = "";
String current_time;
String current_temperature;
String current_rain;
time_t system_start_time;
time_t last_weather_update = -INF;
time_t last_bus_update = -INF;

unsigned int heartbeat_counter = 1;
bool initialized = false;

void setup() {
    Serial.begin(115200);
    Serial.println("Hi Serial");
    drawer.begin();
    drawer.setFont(u8g2_font_7x14B_mf);
    connect_wifi();
    Serial.println("Wait before starting time;");
    delay(3000);
    set_starting_time();
    Serial.print("Starting time: ");
    Serial.println(starting_time);
    Serial.println("Timer set to " + String(TIMER_DELAY / 1000) + " seconds");
}

void connect_wifi() {
    WiFi.begin(ssid, password);
    Serial.println("Connecting");
    int retries = 100;
    while (WiFi.status() != WL_CONNECTED and retries > 0) {
        delay(500);
        drawer.home();
        drawer.clearDisplay();
        Serial.print(".");
        print_line(drawer, "Connecting to Wifi");
        print_line(drawer, "Retries left: " + String(retries));
        drawer.sendBuffer();
        retries--;
    }

    Serial.println("Connected to WiFi network with IP Address: ");
    Serial.println(WiFi.localIP());
}

int set_starting_time() {
    Serial.println("Setting starting time");
    JSONVar response;
    int retries = 30;
    int response_failure = 1;
    while (retries && (response_failure = get_time_json(response))) {
        retries -= 1;
        delay(3000);
        print_line(drawer, "get_time_json " + String(retries));
        // Serial.println("Printed to serail amout: " + String(response.printTo(Serial)));
    }
    if (response_failure) {
        return 1;
    }
    starting_time = (String)response["datetime"];
    time(&system_start_time);
    Serial.println("Starting time set " + starting_time);
    return 0;
}

int get_busses_json(JSONVar &response) {
    return get_json_https(BUSSES_API_PATH, response);
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
        update_system_time();
    }
    else{
        print_line(drawer, "WiFi not connected");
    }

    drawer.sendBuffer();
    initialized = true;
    delay(5000);
    drawer.clearDisplay();
}

void heartbeat_to_serial() {
    Serial.println("HeartBeat " + String(heartbeat_counter));
    heartbeat_counter++;
}

void update_system_time(){
    time_t now;
    time(&now);
    Serial.println("Updating system time" + String(now));
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    time(&now);
    while (now < 100000) {
        Serial.println(String(now));
        delay(500);
        time(&now);
    }
    time(&system_start_time);
    Serial.println("System time after update: " + String(system_start_time));
}

// TODO: get rid of this and reuse `update_system_time`
void update_current_time() {
    time_t system_current_time;
    time(&system_current_time);
    int seconds = difftime(system_current_time, system_start_time);

    tm t;
    strptime(starting_time.c_str(), "%Y-%m-%dT%T", &t);
    t.tm_sec += seconds;

    // Normalization
    mktime(&t);

    char buf[sizeof "1970-01-01T00:00:00"];
    strftime(buf, sizeof buf, "%Y-%m-%dT%H:%M:%S", &t);
    current_time = String(buf);
}

int upper_bound_time(String start_time, std::vector<String> &times) {
    return std::upper_bound(times.begin(), times.end(), start_time) - times.begin();
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

int update_weather_info(bool fetch_required){
    Serial.println("Update weather");

    JSONVar weather;
    if (fetch_required){
        int rc = get_weather_json(weather);
        if (rc != 0){ return rc; }
        time(&last_weather_update);

        // E.g. weather API returned times [7:00, 7:01, 7:02, ...] and current_time 
        // is 7:00:35. So we should do these calculations
        std::vector<String> times = get_strings_from_json(weather["hourly"]["time"]);
        int next_time_point = upper_bound_time(current_time, times);
        int cur_time_point = next_time_point > 0 ? next_time_point - 1 
                                : next_time_point;
        
        update_temperature(cur_time_point, weather);
        update_rains_info(cur_time_point, next_time_point, times, weather);
    }


    // char(176) is a degree symbol °
    print_line(drawer, "Now: " + to_humanreadable(current_time) + " " + 
               current_temperature + String(char(176)) + "C");
    print_line(drawer, current_rain);
    return 0;
}

void update_rains_info(int cur_time_point, int next_time_point, 
                       std::vector<String> &times, JSONVar &weather){
    std::vector<float> rains = get_floats_from_json(weather["hourly"]["precipitation"]);
        
    bool is_raining = rains[cur_time_point] > 0;
    next_time_point = find_next(next_time_point, rains, is_raining);

    if (is_raining) {
        if (next_time_point == -1) {
            current_rain = "Rain forever";
        } else {
            current_rain = "Rain stops: " + to_humanreadable(times[next_time_point]);
        }
    } else {
        if (next_time_point == -1 || next_time_point - cur_time_point > 24) {
            current_rain = "No rains today";
        } else {
            current_rain = "Starts: " + to_humanreadable(times[next_time_point]);
            tm cur_time = convert_iso8601_to_tm(times[cur_time_point]);
            tm rain_time = convert_iso8601_to_tm(times[next_time_point]);
            if (cur_time.tm_hour > rain_time.tm_hour) {
                current_rain += " tmrw";
            }
        }
    }
}

int check_correct_bus(JSONVar &busses){
    return String(busses[bustpc]["Stop"]["TimingPointName"]) == busstop_name;
}

int update_busses_info(bool fetch_required){
    Serial.println("Update busses");

    JSONVar busses;
    // TODO:
    // To save memory, `fetch_required` must be used like in `update_weather_info`
    int rc = get_busses_json(busses);
    if (rc != 0){ return rc; }

    time(&last_bus_update);

    if (!check_correct_bus(busses)){
        return 505;
    }

    JSONVar passes = busses[bustpc]["Passes"];
    JSONVar keys = passes.keys();

    std::vector<String> arrival_times;

    for (int i = 0; i < keys.length(); i++){
        JSONVar pass = passes[keys[i]];
        
        if (String(pass["DestinationName50"]) == busstop_destination){
            arrival_times.push_back(String(pass["ExpectedArrivalTime"]));
        }
    }

    std::sort(arrival_times.begin(), arrival_times.end());

    int busses_added = 0;
    tm cur_time = convert_iso8601_to_tm(current_time);
    time_t cur_time_sec = mktime(&cur_time);

    for (int i = 0; i < arrival_times.size() && busses_added <= BUSSES_TO_SHOW; i++){
        tm bus_time = convert_iso8601_to_tm(arrival_times[i]);
        time_t bus_time_sec = mktime(&bus_time);

        int diff_in_secs = bus_time_sec - cur_time_sec;
        if (diff_in_secs < 0){
            Serial.println("Some issue with bus times");
            Serial.println("arrival_times[i] " + String(arrival_times[i]));
            Serial.println("bus_time_sec " + String(bus_time_sec));
            Serial.println("cur_time_sec " + String(cur_time_sec));
            continue;
        }
        int diff_in_mins = diff_in_secs / 60;
        print_line(drawer, "3 bus in: " + String(diff_in_mins) + "min");
        busses_added++;
    }
    return 0;
}

int refresh_data_if_needed() {
    time_t now;
    time(&now);

    update_current_time();
    
    bool weather_data_update_required = difftime(now, last_weather_update) >= WEATHER_API_DELAY;
    bool bus_data_update_required = difftime(now, last_bus_update) >= BUS_API_DELAY;

    int weather_response_code = 0;
    int bus_response_code = 0;
    weather_response_code = update_weather_info(weather_data_update_required);
    bus_response_code = update_busses_info(bus_data_update_required);
    return weather_response_code || bus_response_code;
}

String update_temperature(int cur_time_point, JSONVar &weather){
    std::vector<float> temps = get_floats_from_json(
        weather["hourly"]["temperature_2m"]);
    current_temperature = String(int(temps[cur_time_point]));
    return current_temperature;
}

int parse_and_print() {
    if (WiFi.status() == WL_CONNECTED) {
        return refresh_data_if_needed();
    } else {
        print_line(drawer, "WiFi not connected");
        Serial.println("WiFi Disconnected");
    }
    return 0;
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
    delay(TIMER_DELAY);
}
