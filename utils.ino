#include "utils.h"

tm convert_iso8601_to_tm(String time_in_iso8601) {
    // "2023-10-08T20:15:21.479523+02:00"
    // "2023-10-08T00:00"
    tm t;
    // strptime(time_in_iso8601.c_str(), "%Y-%m-%dT%H:%M", &t);
    strptime(time_in_iso8601.c_str(), "%Y-%m-%dT%T", &t);
    return t;
}

String to_humanreadable(String time_in_iso8601) {
    tm t = convert_iso8601_to_tm(time_in_iso8601);
    char buf[sizeof "07:07"];
    strftime(buf, sizeof buf, "%H:%M", &t);
    return String(buf);
}