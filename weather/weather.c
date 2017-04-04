#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <libgen.h>
#include <time.h>

#include <ao/ao.h>

#include <nemotool.h>
#include <nemotimer.h>
#include <nemoshow.h>

#include "nemoutil.h"
#include "widgets.h"
#include "nemoui.h"

#define UPDATE_WINDOW 1

#define TRANS_TIME  1000 * 10
#define UPDATE_TIME 1000 * 60 * 30

#define NOW_TIME 1000 * 13

#define PATH_SIZE 512

#define COLOR_BASE 0x1EDCDCFF
#define COLOR_TEXT 0xBBF4F4FF

/****************************
 * Open Weather
 * **************************/
#define OPENWEATHER_PREFIX "api.openweathermap.org/data/2.5/"
#define OPENWEATHER_APPID "APPID=1d78cf9f9456acfbaad4e6cd4e6cef9d&"

typedef enum {
    OPENWEATHER_TYPE_NOW,
    OPENWEATHER_TYPE_HOURS,
    OPENWEATHER_TYPE_DAILY
} OPENWEATHER_TYPE;

const char *OPENWEATHER_PREFIX_NOW = OPENWEATHER_PREFIX"weather?"OPENWEATHER_APPID;
const char *OPENWEATHER_PREFIX_HOURS = OPENWEATHER_PREFIX"forecast?"OPENWEATHER_APPID;
const char *OPENWEATHER_PREFIX_DAILY = OPENWEATHER_PREFIX"forecast/daily?"OPENWEATHER_APPID;

static char *
_openweather_url_get(OPENWEATHER_TYPE type, const char *city, const char *country, int cityid, double lat, double lon)
{
    const char *prefix;
    const char *postfix;
    if (type == OPENWEATHER_TYPE_NOW) {
        prefix = OPENWEATHER_PREFIX_NOW;
        postfix = "&units=metric";
    } else if (type == OPENWEATHER_TYPE_HOURS) {
        prefix = OPENWEATHER_PREFIX_HOURS;
        postfix = "&units=metric";
    } else if (type == OPENWEATHER_TYPE_DAILY) {
        prefix = OPENWEATHER_PREFIX_DAILY;
        postfix = "&units=metric&cnt=10";
    }
    char *ret;
    if (city) {
        if (country) {
            ret = strdup_printf("%sq=%s,%s%s", prefix, city, country, postfix);
        } else {
            ret = strdup_printf("%sq=%s%s", prefix, city, postfix);
        }
    } else if (cityid >= 0) {
        ret = strdup_printf("%sid=%d%s", prefix, cityid, postfix);
    } else {
        ret = strdup_printf("%slat=%lf&lon=%lf%s", prefix, lat, lon, postfix);
    }
    return ret;
}

typedef struct _WeatherDay WeatherDay;
struct _WeatherDay
{
    int datetime;
    double temp;
    double temp_min;
    double temp_max;
    int pressure;
    int humidity;
    int weather_id;
    char *weather_main;
    char *weather_desc;
};

typedef struct _WeatherDaily WeatherDaily;
struct _WeatherDaily {
    char *city;             // name:             City name
    double lat, lon;        // coord/lat, coord/lon
    char *country;          // sys/country
    int days_cnt;
    List *days;
};

static inline void _weather_daily_dump(WeatherDaily *w)
{
    ERR("city: %s (%lf %lf)", w->city, w->lat, w->lon);
    ERR("country: %s", w->country);
    ERR("cnt: %d", w->days_cnt);
    List *l;
    WeatherDay *wd;
    LIST_FOR_EACH(w->days, l, wd) {
        ERR("time: %d", wd->datetime);
        ERR(": %d", wd->weather_id);
        ERR("%s, %s", wd->weather_main, wd->weather_desc);
        ERR("temp:%lf, %lf, %lf", wd->temp, wd->temp_min, wd->temp_max);
        ERR("humidity:%d, pressure:%d", wd->humidity, wd->pressure);
    }
}

static void _weather_daily_destroy(WeatherDaily *w)
{
    if (w->city) free(w->city);
    if (w->country) free(w->country);

    WeatherDay *ww;
    LIST_FREE(w->days, ww) {
        if (ww->weather_main) free(ww->weather_main);
        if (ww->weather_desc) free(ww->weather_desc);
        free(ww);
    }
    free(w);
}

static WeatherDaily *_weather_daily_create_openweather(const char *json)
{
    struct json_object *jso = json_tokener_parse(json);
    if (!jso) {
        ERR("json_tokener_parse failed: %s", json);
        return NULL;
    }

    WeatherDaily *w = calloc(sizeof(WeatherDaily), 1);
    Values *vals;

    vals = _json_find(jso, "city/name");
    if (vals) {
        w->city = strdup(values_get_str(vals, 0));
    }

    vals = _json_find(jso, "city/coord/lat");
    if (vals) {
        w->lat = values_get_double(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "city/coord/lon");
    if (vals) {
        w->lon = values_get_double(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "city/country");
    if (vals) {
        w->country = strdup(values_get_str(vals, 0));
    }

    vals = _json_find(jso, "cnt");
    if (vals) {
        w->days_cnt = values_get_double(vals, 0);
        values_free(vals);
    }

    int i = 0;
    for (i = 0 ; i < w->days_cnt ; i++) {
        WeatherDay *day = calloc(sizeof(WeatherDay), 1);
        w->days = list_append(w->days, day);
    }

    vals = _json_find(jso, "list/dt");
    if (vals && (w->days_cnt == vals->cnt)) {
        for (i = 0 ; i < vals->cnt ; i++) {
            WeatherDay *day = LIST_DATA(list_get_nth(w->days, i));
            day->datetime = values_get_double(vals, i);
        }
        values_free(vals);
    }

    vals = _json_find(jso, "list/temp/day");
    if (vals && (w->days_cnt == vals->cnt)) {
        int i = 0;
        for (i = 0 ; i < vals->cnt ; i++) {
            WeatherDay *day = LIST_DATA(list_get_nth(w->days, i));
            day->temp = values_get_double(vals, i);
        }
        values_free(vals);
    }

    vals = _json_find(jso, "list/temp/min");
    if (vals && (w->days_cnt == vals->cnt)) {
        int i = 0;
        for (i = 0 ; i < vals->cnt ; i++) {
            WeatherDay *day = LIST_DATA(list_get_nth(w->days, i));
            day->temp_min = values_get_double(vals, i);
        }
        values_free(vals);
    }
    vals = _json_find(jso, "list/temp/max");
    if (vals && (w->days_cnt == vals->cnt)) {
        int i = 0;
        for (i = 0 ; i < vals->cnt ; i++) {
            WeatherDay *day = LIST_DATA(list_get_nth(w->days, i));
            day->temp_max = values_get_double(vals, i);
        }
        values_free(vals);
    }

    vals = _json_find(jso, "list/pressure");
    if (vals && (w->days_cnt == vals->cnt)) {
        int i = 0;
        for (i = 0 ; i < vals->cnt ; i++) {
            WeatherDay *day = LIST_DATA(list_get_nth(w->days, i));
            day->pressure = values_get_double(vals, i);
        }
        values_free(vals);
    }

    vals = _json_find(jso, "list/humidity");
    if (vals && (w->days_cnt == vals->cnt)) {
        int i = 0;
        for (i = 0 ; i < vals->cnt ; i++) {
            WeatherDay *day = LIST_DATA(list_get_nth(w->days, i));
            day->pressure = values_get_double(vals, i);
        }
        values_free(vals);
    }

    vals = _json_find(jso, "list/weather/id");
    if (vals && (w->days_cnt == vals->cnt)) {
        int i = 0;
        for (i = 0 ; i < vals->cnt ; i++) {
            WeatherDay *day = LIST_DATA(list_get_nth(w->days, i));
            day->weather_id = values_get_int(vals, i);
        }
        values_free(vals);
    }

    vals = _json_find(jso, "list/weather/main");
    if (vals && (w->days_cnt == vals->cnt)) {
        int i = 0;
        for (i = 0 ; i < vals->cnt ; i++) {
            WeatherDay *day = LIST_DATA(list_get_nth(w->days, i));
            day->weather_main = strdup(values_get_str(vals, i));
        }
        values_free(vals);
    }

    vals = _json_find(jso, "list/weather/description");
    if (vals && (w->days_cnt == vals->cnt)) {
        int i = 0;
        for (i = 0 ; i < vals->cnt ; i++) {
            WeatherDay *day = LIST_DATA(list_get_nth(w->days, i));
            day->weather_desc = strdup(values_get_str(vals, i));
        }
        values_free(vals);
    }
    return w;
}

typedef struct _WeatherNow WeatherNow;
struct _WeatherNow {
    // Current weather
    int datetime;           // UNIX UTC
    char *city;             // name:             City name
    double lat, lon;        // coord/lat, coord/lon
    char *country;          // sys/country
    int sunrise;            // sys/sunrise:     UNIX UTC
    int sunset;             // sys/sunset:      UNIX UTC
    int weather_id;         // weather/id
    char *weather_main;     // weather/main:    Rain, Snow, Extreme, etc.
    char *weather_desc;     // weather/description
    double temp;            // main/temp:       Celcius
    double temp_min;
    double temp_max;
    double temp_night;
    double temp_eve;
    double temp_morn;

    int humidity;           // main/humidity:   % (relative)
    int pressure;        // main/pressure:   hPa (Ground)
    double pressure_ground; // main/grnd_level: hpa
    double pressure_sea;    // main/sea_level:  hpa
    double wind_speed;      // wind/speed:      mps
    int wind_degree;     // wind/deg:        0 ~ 360'

    int cloud;           // clouds/all:      %
    double rain;            // rain/3h:         mm for last 3 hours
    double snow;            // snow/3h:         mm for last 3 hours
};

static inline void _weather_now_dump(WeatherNow *w)
{
    ERR("time: %d", w->datetime);
    ERR("city: %s (%lf %lf)", w->city, w->lat, w->lon);
    ERR("country: %s", w->country);
    ERR("sunrise:%d sunset:%d", w->sunrise, w->sunset);
    ERR("ID: %d", w->weather_id);
    ERR("%s, %s", w->weather_main, w->weather_desc);
    ERR("temp:%lf, %lf, %lf", w->temp, w->temp_min, w->temp_max);
    ERR("humidity:%d, pressure:%d", w->humidity, w->pressure);
    ERR("Wind: speed:%lf degree:%d", w->wind_speed, w->wind_degree);
    ERR("cloud:%d, rain:%lf snow:%lf", w->cloud, w->rain, w->snow);
}

static void
_weather_now_destroy(WeatherNow *w)
{
    if (w->city) free(w->city);
    if (w->country) free(w->country);
    if (w->weather_main) free(w->weather_main);
    if (w->weather_desc) free(w->weather_desc);
    free(w);
}

static WeatherNow *
_weather_now_create_openweather(const char *json)
{
    struct json_object *jso = json_tokener_parse(json);
    if (!jso) {
        ERR("json_tokener_parse failed: %s", json);
        return NULL;
    }

    WeatherNow *w = calloc(sizeof(WeatherNow), 1);
    Values *vals;

    vals = _json_find(jso, "name");
    if (vals) {
        w->city = strdup(values_get_str(vals, 0));
    }

    vals = _json_find(jso, "dt");
    if (vals) {
        w->datetime = values_get_int(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "coord/lat");
    if (vals) {
        w->lat = values_get_double(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "coord/lon");
    if (vals) {
        w->lon = values_get_double(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "sys/country");
    if (vals) {
        w->country = strdup(values_get_str(vals, 0));
        values_free(vals);
    }

    vals = _json_find(jso, "sys/sunrise");
    if (vals) {
        w->sunrise = values_get_int(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "sys/sunset");
    if (vals) {
        w->sunset = values_get_int(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "weather/id");
    if (vals) {
        w->weather_id = values_get_int(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "weather/main");
    if (vals) {
        w->weather_main = strdup(values_get_str(vals, 0));
        values_free(vals);
    }

    vals = _json_find(jso, "weather/description");
    if (vals) {
        w->weather_desc = strdup(values_get_str(vals, 0));
        values_free(vals);
    }

    vals = _json_find(jso, "main/temp");
    if (vals) {
        w->temp = values_get_double(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "main/temp_min");
    if (vals) {
        w->temp_min = values_get_double(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "main/temp_max");
    if (vals) {
        w->temp_max = values_get_double(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "main/humidity");
    if (vals) {
        w->humidity = values_get_int(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "main/pressure");
    if (vals) {
        w->pressure = values_get_int(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "main/grnd_level");
    if (vals) {
        w->pressure_ground = values_get_double(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "main/sea_level");
    if (vals) {
        w->pressure_sea = values_get_double(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "wind/speed");
    if (vals) {
        w->wind_speed = values_get_double(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "wind/deg");
    if (vals) {
        w->wind_degree = values_get_int(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "clouds/all");
    if (vals) {
        w->cloud = values_get_int(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "rain/3h");
    if (vals) {
        w->rain = values_get_double(vals, 0);
        values_free(vals);
    }

    vals = _json_find(jso, "snow/3h");
    if (vals) {
        w->snow = values_get_double(vals, 0);
        values_free(vals);
    }
    json_object_put(jso);

    return w;
}
#if 0
typedef struct _OpenWeather_Table OpenWeather_Table;
struct _OpenWeather_Table {
    int id; // openweather id
    const char *str;
};

OpenWeather_Table OPENWEATHER_TABLE[]=
{
    // Thunderstorm (Rain)
    {200, "thunderstorm with light rain"},
    {201, "thunderstorm with rain"},
    {202, "thunderstorm with heavy rain"},
    // Thunderstorm
    {210, "light thunderstorm"},
    {211, "thunderstorm"},
    {212, "heay thunderstorm"},
    {221, "ragged thunderstorm"},
    // Thunderstorm (drizzle)
    {230, "thuderstorm with light drizzle"},
    {231, "thuderstorm with drizzle"},
    {232, "thuderstorm with heavy drizzle"},
    // Rain (drizzle)
    {300, "light intensity drizzle"},
    {301, "drizzle"},
    {302, "heavy intensity drizzle"},
    {310, "light intensity drizzle rain"},
    {311, "drizzle rain"},
    {312, "heavy intensity drizzle rain"},
    {313, "shower rain and drizzle"},
    {314, "heavy shower rain and drizzle"},
    {321, "shower drizzle"},
    // Rain
    {500, "light rain"},
    {501, "moderate rain"},
    {502, "heavy intensity rain"},
    {503, "very heavy rain"},
    {504, "extreme rain"},
    {511, "freezing rain"},
    {520, "light intensity shower rain"},
    {521, "shower rain"},
    {522, "heavy intensity shower rain"},
    {531, "ragged shower rain"},
    // snow
    {600, "light snow"},
    {601, "snow"},
    {602, "heavy snow"},
    {611, "sleet"},
    {612, "shower sleet"},
    {615, "light rain and snow"},
    {616, "rain and snow"},
    {620, "light shower snow"},
    {621, "shower snow"},
    {622, "heavy shower snow"},
    // Atmosphere
    {701, "mist"},  // visibility 1 ~ 2KM (warter droplets)
    {711, "smoke"}, // ? (air pollutants)
    {721, "haze"},  // 2 ~ 5KM  (air pollutants)
    {731, "sand,dust whirls"},
    {741, "fog"},   // < 1KM (water droplets)
    {751, "sand"},
    {761, "dust"},
    {762, "volcanic ash"},
    {771, "squalls"},
    {781, "tornado"},
    // Clouds
    {800, "clear sky"},
    {801, "few cloud"},        // (1/8)
    {802, "scattered clouds"}, // (4/8)
    {803, "broken clouds"},    // (cover between 5/8th ~ 7/8th clouds of the sky.)
    {804, "overcast clouds"},  // (8/8)
    // Extreme // FIXME
    {900, "tornado"},
    {901, "tropical storm"},
    {902, "hurricane"},
    {903, "cold"},
    {904, "hot"},
    {905, "windy"},
    {906, "hail"},
    // Additional
    {951, "calm"},
    {952, "light breeze"},
    {953, "gentle breeze"},
    {954, "moderate breeze"},
    {955, "fresh breeze"},
    {956, "strong breeze"},
    {957, "high wind,near gale"},
    {958, "gale"},
    {959, "severe storm"},
    {960, "storm"},
    {961, "violent storm"},
    {962, "hurricane"}
};
#endif


const char *PATH_CELCIUS = "M209.167,176.375c-3.604-3.396-7.792-6.083-12.542-8.062c-4.812-1.938-9.938-2.938-15.375-2.938 c-5.458,0-10.584,1-15.375,2.938c-4.75,1.979-8.938,4.667-12.541,8.062c-3.584,3.417-6.396,7.417-8.459,12.042 c-2.042,4.605-3.062,9.479-3.062,14.583c0,5.125,1.021,10,3.062,14.605c2.062,4.604,4.875,8.625,8.459,12.02 c3.604,3.417,7.791,6.125,12.541,8.063c4.791,1.979,9.917,2.958,15.375,2.958c5.438,0,10.562-0.979,15.375-2.958 c4.75-1.938,8.938-4.646,12.542-8.063c3.583-3.395,6.396-7.417,8.416-12.02c2.042-4.605,3.104-9.479,3.104-14.605 c0-5.104-1.062-9.979-3.104-14.583C215.562,183.791,212.75,179.791,209.167,176.375z M193.812,215.041 c-3.438,3.417-7.625,5.125-12.562,5.125s-9.125-1.709-12.562-5.125c-3.396-3.417-5.125-7.417-5.125-12.042 c0-4.75,1.729-8.833,5.125-12.145c3.438-3.333,7.625-4.979,12.562-4.979s9.125,1.646,12.562,4.979 c3.375,3.312,5.104,7.395,5.104,12.145C198.916,207.625,197.188,211.625,193.812,215.041z M335.25,311.791 c-6.062,1.73-12.083,2.583-18.062,2.583c-3.938,0-7.812-0.604-11.646-1.79c-3.854-1.209-7.354-3.084-10.499-5.646 c-3.167-2.563-5.73-5.834-7.667-9.854c-1.959-4-2.958-8.917-2.958-14.709v-51.458c0-5.792,0.999-10.75,2.958-14.854 c1.937-4.083,4.459-7.417,7.541-9.979c3.084-2.562,6.542-4.396,10.5-5.5c3.917-1.105,7.834-1.667,11.771-1.667 c5.979,0,12.124,0.875,18.437,2.688c6.313,1.791,12.188,4.999,17.668,9.604l16.896-27.146 c-6.854-5.958-14.979-10.312-24.479-13.062c-9.459-2.708-19.292-4.083-29.543-4.083c-8.354,0-16.583,1.208-24.583,3.584 c-8.021,2.396-15.146,5.979-21.375,10.75c-6.208,4.792-11.271,10.708-15.083,17.791c-3.875,7.084-5.791,15.333-5.791,24.708 v66.042c0,9.562,1.979,17.875,5.916,24.959c3.917,7.084,9.041,12.979,15.334,17.666c6.332,4.708,13.479,8.229,21.499,10.625 c8.042,2.375,16.229,3.584,24.604,3.584c10.396,0,20.271-1.542,29.688-4.604c9.375-3.083,17.312-7.354,23.812-12.792 l-16.646-27.146C347.416,306.875,341.292,310.104,335.25,311.791z";

const char *PATH_FAHRENHEIT = "M208.531,177.271c-3.604-3.396-7.791-6.084-12.541-8.062c-4.812-1.938-9.938-2.938-15.375-2.938 c-5.459,0-10.584,1-15.375,2.938c-4.75,1.979-8.938,4.667-12.542,8.062c-3.583,3.416-6.396,7.416-8.459,12.041 c-2.042,4.605-3.062,9.479-3.062,14.584c0,5.125,1.02,10,3.062,14.604c2.063,4.604,4.876,8.625,8.459,12.021 c3.604,3.416,7.792,6.125,12.542,8.062c4.791,1.979,9.916,2.958,15.375,2.958c5.438,0,10.562-0.979,15.375-2.958 c4.75-1.938,8.938-4.646,12.541-8.062c3.584-3.396,6.396-7.417,8.416-12.021c2.042-4.604,3.105-9.479,3.105-14.604 c0-5.104-1.063-9.979-3.105-14.584C214.927,184.687,212.115,180.687,208.531,177.271z M193.177,215.937 c-3.438,3.417-7.625,5.126-12.562,5.126s-9.125-1.709-12.562-5.126c-3.396-3.416-5.125-7.416-5.125-12.041 c0-4.75,1.729-8.833,5.125-12.146c3.438-3.333,7.625-4.979,12.562-4.979s9.125,1.646,12.562,4.979 c3.374,3.312,5.104,7.396,5.104,12.146C198.281,208.521,196.551,212.521,193.177,215.937z M370.822,200.833v-31.5H249.24v176.396 h35.063v-71.938h73.979v-31.479h-73.979v-41.479H370.822z";

const char *PATH_MERCURY = "M288,360.875V66.917C288,47.625,273.666,32,256,32 c-17.667,0-32,15.625-32,34.917v293.958c-19.042,11.083-32,31.501-32,55.125c0,35.333,28.666,64,64,64c35.333,0,64-28.667,64-64 C320,392.376,307.062,371.958,288,360.875z";

const char *PATH_COMPASS = "M256,0C114.604,0,0,114.604,0,256c0,141.375,114.604,256,256,256c141.375,0,256-114.625,256-256 C512,114.604,397.375,0,256,0z M256,448c-105.875,0-192-86.125-192-192S150.125,64,256,64s192,86.125,192,192S361.875,448,256,448 M160,352l128-64l64-128l-128,64L160,352z";

const char *PATH_NA = "M159.094,278.646h-1.042l-33.521-51.188l-40.938-60.437h-31.75v176.395h35.062V232.312h1.521 l29.104,44.542l44.688,66.562h31.938V167.021h-35.062V278.646z M206.469,354.166h25.834l64.791-196.332h-26.125L206.469,354.166z M398.719,167.021h-35.833l-61.979,176.395h37.375l11.521-36.375h61.459l10.749,36.375h38.146L398.719,167.021z M359.281,277.625 l7.146-23.292l13.604-45.832h1l13.854,46.604l6.896,22.521H359.281z";

// 443.615x443.615
const char *PATH_PERCENT = "M209.949,100.813C209.949,45.232,164.725,0,109.144,0C53.562,0,8.346,45.232,8.346,100.813 c0,55.566,45.216,100.782,100.798,100.782C164.725,201.596,209.949,156.38,209.949,100.813z M58.419,100.813 c0-27.986,22.754-50.74,50.725-50.74c27.969,0,50.732,22.754,50.732,50.74c0,27.954-22.763,50.709-50.732,50.709 C81.173,151.522,58.419,128.768,58.419,100.813z M334.473,242.02c-55.583,0-100.799,45.216-100.799,100.782c0,55.582,45.216,100.814,100.799,100.814 c55.581,0,100.797-45.231,100.797-100.814C435.27,287.235,390.054,242.02,334.473,242.02z M334.473,393.542 c-27.971,0-50.726-22.755-50.726-50.74c0-27.954,22.755-50.709,50.726-50.709c27.969,0,50.724,22.755,50.724,50.709 C385.196,370.787,362.441,393.542,334.473,393.542z M337.577,12.925c-13.081-7.254-29.616-2.574-36.91,10.515L94.896,392.923c-7.294,13.089-2.59,29.616,10.505,36.919 c4.181,2.314,8.713,3.423,13.179,3.423c9.527,0,18.77-5.02,23.733-13.936L348.083,49.845 C355.376,36.755,350.675,20.228,337.577,12.925z";

// 401.949 x 401.949
const char *PATH_UP = "M328.508,173.212L211.214,4.948c-5.633-6.598-14.846-6.598-20.479,0L73.445,173.209 c-5.631,6.599-3.146,11.996,5.529,11.996h49.068c8.672,0,15.77,7.097,15.77,15.771l0.077,51.518v133.428l-0.021,0.292 c0.003,8.676,7.321,15.734,15.991,15.736l82.789-0.002c8.674,0,15.771-7.096,15.771-15.766l-0.279-185.207 c0-8.674,7.094-15.771,15.769-15.771h49.066C331.647,185.205,334.136,179.808,328.508,173.212z";

// 537.794 x 537.795
const char *PATH_DOWN = "M463.091,466.114H74.854c-11.857,0-21.497,9.716-21.497,21.497v28.688c0,11.857,9.716,21.496,21.497,21.496h388.084 c11.857,0,21.496-9.716,21.496-21.496v-28.688C484.665,475.677,474.949,466.114,463.091,466.114 M253.94,427.635c4.208,4.208,9.716,6.35,15.147,6.35c5.508,0,11.016-2.142,15.147-6.35l147.033-147.033	c8.339-8.338,8.339-21.955,0-30.447l-20.349-20.349c-8.339-8.339-21.956-8.339-30.447,0l-75.582,75.659V21.497 C304.889,9.639,295.173,0,283.393,0h-28.688c-11.857,0-21.497,9.562-21.497,21.497v284.044l-75.658-75.659 c-8.339-8.338-22.032-8.338-30.447,0l-20.349,20.349c-8.338,8.338-8.338,22.032,0,30.447L253.94,427.635z";

// 612x612
const char *PATH_PINWHEEL ="M612,293.728H447.507V154.556L293.727,0.001v164.493H154.555L0,318.274h164.493v139.172l153.778,154.553V447.506h139.172	L612,293.728z M552.529,318.274l-100.128,99.625l-93.271-93.271c0.726-2.067,1.332-4.187,1.818-6.356h191.581V318.274z M306,337.767c-17.515,0-31.766-14.249-31.766-31.766s14.249-31.766,31.766-31.766c17.517,0,31.766,14.249,31.766,31.766 S323.515,337.767,306,337.767z M360.948,293.728c-2.356-10.556-7.672-20.007-15.047-27.42l77.058-77.058v104.478L360.948,293.728	L360.948,293.728z M318.273,59.472L417.9,159.6l-93.271,93.271c-2.067-0.726-4.187-1.332-6.356-1.816V59.472z M293.727,251.053 c-10.556,2.356-20.007,7.672-27.42,15.045l-77.058-77.058h104.476v62.014H293.727z M59.471,293.728l100.128-99.625l93.271,93.271	c-0.726,2.067-1.332,4.187-1.816,6.356H59.471V293.728z M251.052,318.274c2.356,10.556,7.672,20.007,15.045,27.42l-77.058,77.058 V318.274H251.052z M293.727,552.53l-99.625-100.126l93.271-93.271c2.067,0.726,4.187,1.332,6.356,1.816V552.53H293.727z M318.273,360.949c10.556-2.356,20.007-7.672,27.42-15.047l77.058,77.058H318.273V360.949L318.273,360.949z";

// 792x792
const char *PATH_THERMOMETER = "M420.75,403.376V111.375C420.75,49.97,370.78,0,309.375,0S198,49.97,198,111.375v292.001 c-60.811,38.164-99,105.731-99,178.249C99,697.628,193.372,792,309.375,792c116.003,0,210.375-94.372,210.375-210.375	C519.75,509.107,481.561,441.54,420.75,403.376z M309.375,717.75c-75.067,0-136.125-61.083-136.125-136.125	c0-52.099,30.542-100.337,77.839-122.834c12.944-6.163,21.161-19.206,21.161-33.512v-91.154 c0-20.468,16.657-37.125,37.125-37.125s37.125,16.657,37.125,37.125v91.154c0,14.306,8.242,27.373,21.161,33.512 c47.298,22.497,77.839,70.735,77.839,122.834C445.5,656.667,384.442,717.75,309.375,717.75z M495,321.75h198V247.5H495V321.75z M495,99v74.25h198V99H495z";

// 51.619x51.619
const char *PATH_UMBRELLA ="M12.037,10.25c2.002,0,3.625-1.623,3.625-3.625C15.662,4.042,12.037,0,12.037,0S8.412,4.042,8.412,6.625 C8.412,8.627,10.035,10.25,12.037,10.25z M5.598,4.036C5.598,2.463,3.389,0,3.389,0S1.181,2.463,1.181,4.036c0,1.22,0.989,2.209,2.208,2.209 S5.598,5.256,5.598,4.036z M28.674,2.881V2.627c0-0.957-0.796-1.733-1.775-1.733c-0.981,0-1.776,0.776-1.776,1.733v0.256 C22.374,3.09,19.769,3.79,17.365,4.86c0.185,0.599,0.298,1.195,0.298,1.764c0,3.102-2.523,5.625-5.625,5.625 c-1.172,0-2.26-0.361-3.162-0.976C5.51,15.3,3.449,20.456,3.395,26.104c0.062-3.19,2.665-5.763,5.872-5.763 c3.248,0,5.88,2.632,5.88,5.878c0-3.247,2.632-5.878,5.878-5.878c1.226,0,2.363,0.376,3.305,1.017v23.368 c0,3.742,3,6.8,6.721,6.892l0.172,0.002v-0.002c3.722-0.092,6.721-3.146,6.721-6.892c0-1.381-1.117-2.5-2.5-2.5 c-1.381,0-2.5,1.119-2.5,2.5c0,1.016-0.803,1.847-1.807,1.892c-1.004-0.045-1.808-0.876-1.808-1.892V21.471 c0.971-0.706,2.161-1.128,3.452-1.128c3.248,0,5.88,2.632,5.88,5.877c0-3.246,2.633-5.877,5.879-5.877	c3.248,0,5.879,2.632,5.879,5.877c0,0.039,0.02,0.114,0.02,0.114C50.437,13.934,40.843,3.783,28.674,2.881z";

// 67x67 (Fill)
const char *PATH_SUN[]= {
    "M34.064,54.477c-11.671,0-21.167-9.494-21.167-21.166c0-11.67,9.495-21.166,21.167-21.166 \
         c11.672,0,21.168,9.495,21.168,21.166C55.232,44.982,45.736,54.477,34.064,54.477z M34.064,16.146 \
         c-9.465,0-17.167,7.7-17.167,17.167c0,9.465,7.701,17.166,17.167,17.166c9.467,0,17.168-7.699,17.168-17.166 \
         C51.232,23.846,43.531,16.146,34.064,16.146z",
    "M23.097,26.972c-0.174,0-0.35-0.045-0.51-0.14c-0.475-0.282-0.631-0.896-0.35-1.371c2.07-3.488,5.875-5.655,9.929-5.655 \
        c0.553,0,1,0.447,1,1c0,0.553-0.447,1-1,1c-3.352,0-6.497,1.792-8.208,4.677C23.771,26.797,23.438,26.972,23.097,26.972z",
    "M21.625,31.979c-0.553,0-1-0.447-1-1c0-0.933,0.109-1.444,0.304-2.27c0.126-0.538,0.673-0.868,1.202-0.745 \
        c0.538,0.127,0.871,0.665,0.745,1.202c-0.174,0.74-0.251,1.097-0.251,1.813C22.625,31.532,22.178,31.979,21.625,31.979z",
    "M34.87,67c-1.104,0-2-0.896-2-2v-6c0-1.105,0.896-2,2-2c1.104,0,2,0.895,2,2v6C36.87,66.104,35.975,67,34.87,67z",
    "M34.87,10c-1.104,0-2-0.896-2-2V2c0-1.104,0.896-2,2-2c1.104,0,2,0.896,2,2v6C36.87,9.104,35.975,10,34.87,10z",
    "M11.727,57.584c-0.512,0-1.023-0.195-1.414-0.586c-0.781-0.781-0.781-2.047,0-2.828l4.242-4.242 \
        c0.781-0.781,2.047-0.781,2.828,0c0.781,0.781,0.781,2.047,0,2.828l-4.242,4.242C12.75,57.389,12.238,57.584,11.727,57.584z",
    "M52.031,17.28c-0.512,0-1.023-0.195-1.414-0.586c-0.781-0.781-0.781-2.047,0-2.828l4.242-4.242 \
        c0.781-0.781,2.047-0.781,2.828,0c0.78,0.781,0.78,2.047,0,2.828l-4.242,4.242C53.055,17.085,52.543,17.28,52.031,17.28z",
    "M15.969,17.28c-0.512,0-1.023-0.195-1.414-0.586l-4.242-4.242c-0.781-0.781-0.781-2.047,0-2.828 \
        c0.781-0.781,2.047-0.781,2.828,0l4.242,4.242c0.781,0.781,0.781,2.047,0,2.828C16.992,17.085,16.48,17.28,15.969,17.28z",
    "M56.273,57.586c-0.512,0-1.023-0.195-1.414-0.586l-4.242-4.244c-0.781-0.781-0.781-2.047,0-2.828s2.047-0.781,2.828,0 \
        l4.242,4.244c0.78,0.781,0.78,2.047,0,2.828C57.297,57.391,56.785,57.586,56.273,57.586z",
    "M8,36H2c-1.104,0-2-0.896-2-2c0-1.104,0.896-2,2-2h6c1.104,0,2,0.896,2,2C10,35.104,9.104,36,8,36z",
    "M65,36h-6c-1.104,0-2-0.896-2-2c0-1.104,0.896-2,2-2h6c1.104,0,2,0.896,2,2C67,35.104,66.104,36,65,36z",
    NULL
};

// Moon (Fill, 612x612)
const char *PATH_MOON =
"M604.851,370.662c-3.5-2.85-7.784-4.284-12.106-4.284c-3.213,0-6.445,0.803-9.371,2.429 c-35.993,20.081-77.456,31.556-121.616,31.556c-138.159,0-250.117-111.958-250.117-250.116c0-44.16,11.475-85.642,31.556-121.635 c3.825-6.866,3.099-15.376-1.855-21.477C237.651,2.543,232.124,0,226.405,0c-1.894,0-3.806,0.287-5.661,0.86	C93.104,40.182,0.003,158.948,0.003,299.364C0.003,471.757,140.266,612,312.659,612c140.396,0,259.183-93.101,298.484-220.722 C613.457,383.763,610.97,375.596,604.851,370.662z M312.659,554.625c-140.76,0-255.28-114.501-255.28-255.261	c0-82.333,38.786-156.863,101.821-203.93c-3.27,18.035-4.934,36.395-4.934,54.812c0,169.543,137.93,307.491,307.492,307.491	c18.437,0,36.777-1.663,54.812-4.934C469.503,515.858,394.992,554.625,312.659,554.625z";

// Cloud (19x19)
const char *PATH_CLOUD =
"M16.122,9.194h-0.077c-0.21-1.844-1.773-3.277-3.675-3.277c-0.657,0-1.273,0.174-1.81,0.473 c-0.707-1.467-2.211-2.48-3.952-2.48c-2.422,0-4.388,1.965-4.388,4.387c0,0.336,0.039,0.664,0.111,0.979 C0.996,9.586,0,10.781,0,12.211c0,1.664,1.351,3.014,3.015,3.014l0.489-0.021H15.59l0.531,0.021c1.664,0,3.014-1.35,3.014-3.014 S17.786,9.195,16.122,9.194z";
// Cloud (Fill, 32x32)
const char *PATH_CLOUD2 =
"M27.586,14.212C26.66,11.751,24.284,10,21.5,10c-0.641,0-1.26,0.093-1.846,0.266 C18.068,7.705,15.233,6,12,6c-4.905,0-8.893,3.924-8.998,8.803C1.208,15.842,0,17.783,0,20c0,3.312,2.687,6,6,6h20 c3.312,0,6-2.693,6-6C32,17.234,30.13,14.907,27.586,14.212z M26.003,24H5.997C3.794,24,2,22.209,2,20 c0-1.893,1.318-3.482,3.086-3.896C5.03,15.745,5,15.376,5,15c0-3.866,3.134-7,7-7c3.162,0,5.834,2.097,6.702,4.975	C19.471,12.364,20.441,12,21.5,12c2.316,0,4.225,1.75,4.473,4h0.03C28.206,16,30,17.791,30,20C30,22.205,28.211,24,26.003,24z";

// Rain (Fill, 792x792)
const char *PATH_RAIN =
"M665.335,486.777c-7.815-46.161-25.534-88.323-43.162-126.578C569.197,243.434,505.408,137.391,442.619,39.255\
        l-7.815-13.721C423.991,8.814,411.269,0,396.549,0c-21.626,0-35.347,19.627-39.255,25.534 \
        c0,0,0,0,0,1L343.573,48.16 c-24.534,39.255-50.068,80.508-74.602,121.671 \
        C230.716,234.62,182.647,320.944,147.3,413.174 c-11.813,30.441-22.535,60.881-23.535,94.229 \
        c-3.907,86.324,27.442,159.018,92.23,215.901C266.063,767.466,329.852,792,395.549,792 l0,0 \
        c96.138,0,183.552-49.068,233.529-132.485C662.427,604.541,675.148,545.659,665.335,486.777z \
        M597.638,640.888 c-43.162,72.603-118.764,114.765-202.18,114.765 \
        c-56.883,0-112.857-20.627-156.019-58.882 c-55.974-49.068-83.416-112.857-80.508-187.459 \
        c1-27.442,9.814-53.975,21.626-82.417c34.348-90.322,81.417-174.647,118.764-238.436 \
        c23.535-40.254,49.068-81.417,74.602-120.672l12.721-20.627c0-1,1-1,1-1.999 \
        c1.999-2.908,5.906-7.815,7.815-8.814 c0,0,2.908,1,7.815,8.814l7.815,12.721 \
        c60.881,96.138,124.67,201.18,176.646,316.037c16.72,37.256,33.348,76.51,40.254,117.764 \
        C637.893,542.751,627.079,592.728,597.638,640.888z M413.087,662.423 \
        c0,9.814-7.815,17.628-17.628,17.628	c-89.323,0-160.926-72.603-160.926-160.926 \
        c0-9.814,7.815-17.628,17.628-17.628c9.814,0,17.628,7.815,17.628,17.628 \
        c0.999,68.696,56.974,124.67,125.669,124.67C405.272,643.795,413.087,652.609,413.087,662.423z";
// Rain (847x847)
const char *PATH_DRIZZLE =
"M406.269,10.052l-232.65,405.741c-48.889,85.779-52.665,194.85,0,286.697 \
        c79.169,138.07,255.277,185.82,393.348,106.65 \
		c138.071-79.169,185.821-255.276,106.651-393.348L440.968,10.052 \
        C433.283-3.351,413.953-3.351,406.269,10.052z";

// Snow (Fill, 32x32)
const char *PATH_SNOW =
"M31.996,15.854c-0.005-0.552-0.457-0.996-1.01-0.991l-2.582,0.023l2.271-2.312c0.389-0.395,0.383-1.027-0.013-1.414 c-0.395-0.387-1.024-0.382-1.414,0.013l-3.672,3.739l-7.174,0.064l4.953-5.044l5.245-0.046c0.553-0.005,0.996-0.457,0.99-1.009	c-0.005-0.553-0.456-0.996-1.009-0.991l-3.243,0.029l1.885-1.917c0.387-0.395,0.381-1.027-0.015-1.414 c-0.395-0.387-1.026-0.382-1.414,0.013l-1.881,1.915l-0.029-3.241c-0.006-0.552-0.456-0.996-1.01-0.991 c-0.552,0.005-0.996,0.457-0.99,1.01l0.048,5.24l-4.956,5.046l-0.064-7.17l3.675-3.741c0.389-0.395,0.382-1.027-0.013-1.414 c-0.395-0.387-1.026-0.381-1.414,0.013l-2.273,2.314l-0.021-2.586C16.857,0.438,16.406-0.004,15.854,0 c-0.552,0.005-0.996,0.456-0.99,1.009l0.023,2.584l-2.312-2.271c-0.395-0.387-1.027-0.382-1.414,0.013 c-0.388,0.395-0.382,1.027,0.012,1.414l3.739,3.673l0.064,7.172L9.932,8.641L9.885,3.396C9.88,2.844,9.428,2.4,8.876,2.406	c-0.552,0.006-0.995,0.456-0.991,1.01l0.029,3.243L5.998,4.774c-0.394-0.386-1.027-0.38-1.414,0.013 C4.197,5.184,4.202,5.814,4.597,6.203l1.915,1.882L3.271,8.112c-0.552,0.006-0.996,0.456-0.992,1.01 c0.006,0.552,0.457,0.996,1.01,0.991l5.241-0.047l5.044,4.954l-7.172,0.063l-3.738-3.672c-0.395-0.387-1.027-0.381-1.414,0.014 c-0.388,0.394-0.382,1.026,0.012,1.413l2.312,2.271L0.99,15.132C0.438,15.139-0.006,15.589,0,16.143 c0.004,0.552,0.456,0.995,1.009,0.99l2.585-0.022L1.32,19.424c-0.387,0.396-0.381,1.025,0.014,1.414 c0.197,0.191,0.453,0.289,0.709,0.285c0.256-0.002,0.512-0.102,0.705-0.299l3.674-3.74l7.17-0.063l-4.955,5.045l-5.24,0.047 c-0.553,0.006-0.997,0.457-0.993,1.01c0.006,0.553,0.457,0.996,1.01,0.99l3.24-0.029L4.773,26 c-0.387,0.396-0.381,1.025,0.013,1.414c0.197,0.193,0.454,0.289,0.71,0.287C5.752,27.697,6.007,27.597,6.2,27.4l1.882-1.916 l0.029,3.244c0.006,0.551,0.457,0.996,1.01,0.989c0.552-0.004,0.996-0.457,0.99-1.01l-0.047-5.242l4.954-5.045l0.064,7.174 l-3.673,3.738c-0.387,0.396-0.381,1.027,0.013,1.414s1.026,0.383,1.414-0.014l2.271-2.312l0.022,2.584	c0.006,0.552,0.457,0.996,1.01,0.99c0.552-0.004,0.996-0.457,0.99-1.01l-0.023-2.585l2.313,2.272 c0.197,0.193,0.454,0.289,0.711,0.287s0.511-0.104,0.703-0.299c0.39-0.396,0.382-1.027-0.012-1.414l-3.741-3.676l-0.062-7.171	l5.045,4.957L22.11,28.6c0.006,0.551,0.457,0.993,1.01,0.989c0.552-0.006,0.994-0.457,0.99-1.01l-0.028-3.24l1.915,1.881 c0.197,0.193,0.454,0.289,0.71,0.287c0.256-0.004,0.511-0.104,0.704-0.301c0.388-0.395,0.382-1.024-0.014-1.414l-1.916-1.881 l3.242-0.029c0.553-0.006,0.996-0.457,0.991-1.01c-0.006-0.553-0.457-0.996-1.01-0.99l-5.242,0.048l-5.045-4.957l7.172-0.062 l3.74,3.674c0.197,0.193,0.454,0.289,0.71,0.287s0.511-0.104,0.704-0.299c0.388-0.396,0.382-1.027-0.012-1.414l-2.314-2.273 l2.586-0.022C31.56,16.858,32.002,16.408,31.996,15.854z";

// Thunder (368 x 368)
const char *PATH_THUNDER =
"M297.51,150.349c-1.411-2.146-3.987-3.197-6.497-2.633 l-73.288,16.498 L240.039,7.012c0.39-2.792-1.159-5.498-3.766-6.554  c-2.611-1.069-5.62-0.216-7.283,2.054 L71.166,217.723  c-1.489,2.035-1.588,4.773-0.246,6.911 c1.339,2.132,3.825,3.237,6.332,2.774 l79.594-14.813l-23.257,148.799 c-0.436,2.798,1.096,5.536,3.714,6.629c0.769,0.312,1.562,0.469,2.357,0.469  c1.918,0,3.78-0.901,4.966-2.517l152.692-208.621 C298.843,155.279,298.916,152.496,297.51,150.349z";

// Wind (Stroking, 46x46)
const char *PATH_WIND[]= {
"M40.01,16.291c0-3.771-3.07-6.844-6.841-6.844s-6.839,3.065-6.839,6.837c0,1.078,0.874,1.95,1.951,1.95 c1.078,0,1.951-0.874,1.951-1.952c0-1.618,1.316-2.936,2.937-2.936s2.937,1.318,2.937,2.938 c0,2.147-1.746,3.896-3.896,3.896 H1.953c-1.078,0-1.952,0.881-1.952,1.959 c0,1.078,0.874,1.958,1.952,1.958h30.258C36.51,24.097,40.01,20.591,40.01,16.291z",
    "M1.464,16.162h10.843c3.655,0,6.629-2.986,6.629-6.643c0-3.188-2.594-5.789-5.782-5.789 S7.371,6.321,7.371,9.51 c0,0.809,0.655,1.463,1.464,1.463c0.808,0,1.464-0.656,1.464-1.465 c0-1.574,1.28-2.855,2.854-2.855 c1.574,0,2.855,1.288,2.855,2.862c0,2.042-1.661,3.71-3.702,3.71 H1.464C0.655,13.225,0,13.885,0,14.694S0.656,16.162,1.464,16.162z",
    "M38.066,27.498H9.758c-1.077,0-1.951,0.881-1.951,1.959c0,1.077,0.874,1.957,1.951,1.957h28.308 c2.146,0,3.895,1.739,3.895,3.887c0,1.619-1.316,2.934-2.938,2.934c-1.617,0-2.936-1.318-2.936-2.938 c0-1.078-0.873-1.953-1.951-1.953s-1.951,0.873-1.951,1.951c0,3.771,3.067,6.84,6.838,6.84c3.771,0,6.841-3.066,6.841-6.838 C45.864,30.995,42.366,27.498,38.066,27.498z",
    NULL
};

const char *PATH_FOG =
"M401.625,573.75h-191.25c-10.557,0-19.125,8.568-19.125,19.125S199.818,612,210.375,612h95.548c0.019,0,0.058,0,0.077,0 c0.02,0,0.058,0,0.076,0h95.549c10.557,0,19.125-8.568,19.125-19.125S412.182,573.75,401.625,573.75z M601.979,382.5 C608.29,358.001,612,332.469,612,306C612,137.012,475.008,0,306,0S0,137.012,0,306c0,26.469,3.71,52.001,10.021,76.5H601.979z M535.5,497.25h-459c-10.557,0-19.125,8.568-19.125,19.125S65.943,535.5,76.5,535.5h459c10.557,0,19.125-8.568,19.125-19.125 S546.057,497.25,535.5,497.25z M592.875,420.75H19.125C8.568,420.75,0,429.318,0,439.875S8.568,459,19.125,459h573.75 c10.557,0,19.125-8.568,19.125-19.125S603.432,420.75,592.875,420.75z";

typedef struct _Weather Weather;
struct _Weather {
    const char *city;
    const char *country;

    struct nemotool *tool;
    struct nemoshow *show;
    struct showone *parent;

    struct showone *font;
    struct showone *group;

    struct showone *bg;

    struct {
        Con *con;
        int update_window;
        struct nemotimer *update_timer;
        struct nemotimer *timer;
        WeatherNow *w;
        struct showone *group;
        struct showone *group_old;
        struct showone *temp, *temp0, *temp1, *temp_minus;
        struct showone *city_country;
        struct showone *main;
        struct showone *rsc_clip;
        struct showone *rsc_in;
        struct showone *rsc; // rain/snow/cloud
        struct showone *rsc0, *rsc1, *rsc2;
        struct showone *pressure_clip;
        struct showone *pressure_in;
        struct showone *pressure;
        struct showone *pressure0, *pressure1, *pressure2, *pressure3;
        struct showone *wind;
        struct showone *wind0, *wind1, *wind2, *wind3;
        struct showone *humidity_clip;
        struct showone *humidity_in;
        struct showone *humidity;
        struct showone *humidity0, *humidity1, *humidity2;
        struct showone *weather;
        struct showone *weather0;
        struct showone *weather1;
    } now;

    struct {
        Con *con;
        int update_window;
        struct nemotimer *update_timer;
        WeatherDaily *w;
        struct showone *group;
        struct showone *temp_max, *temp_max0, *temp_max1, *temp_max2, *temp_max_minus;
        struct showone *temp_min, *temp_min0, *temp_min1, *temp_min2, *temp_min_minus;
    } daily;
};

static void _path_set_stroke(struct showone *one, uint32_t color, double alpha, int stroke_w)
{
    struct showone *child;
    nemoshow_children_for_each(child, one) {
        nemoshow_item_set_stroke_color(child, RGBA(color));
        nemoshow_item_set_stroke_width(child, stroke_w);
        nemoshow_item_set_alpha(child, alpha);
    }
}

static void _path_set_fill(struct showone *one, uint32_t color, double alpha)
{
    struct showone *child;
    nemoshow_children_for_each(child, one) {
        nemoshow_item_set_fill_color(child, RGBA(color));
        nemoshow_item_set_alpha(child, alpha);
    }
}

static struct showone *_path_create(struct nemoshow *show, struct showone *parent,
        int size,
        const char *cmd, const char **cmds)
{
    struct showone *group, *one;
    double base;
    double pv;

    group = GROUP_CREATE(parent);

    double svg_sz;
    if (cmds == PATH_SUN) {
        svg_sz = 67;
    } else if (cmds == PATH_WIND) {
        svg_sz = 46;
    } else if (cmd == PATH_MOON) {
        svg_sz = 612;
    } else if (cmd == PATH_FOG) {
        svg_sz = 612;
    } else if (cmd == PATH_CLOUD) {
        svg_sz = 19;
    } else if (cmd == PATH_CLOUD2) {
        svg_sz = 32;
    } else if (cmd == PATH_RAIN) {
        svg_sz = 792;
    } else if (cmd == PATH_DRIZZLE) {
        svg_sz = 842;
    } else if (cmd == PATH_SNOW) {
        svg_sz = 32;
    } else if (cmd == PATH_THUNDER) {
        svg_sz = 368;
    } else if (cmd == PATH_PERCENT) {
        svg_sz = 443.615;
    } else if (cmd == PATH_UP) {
        svg_sz = 401.949;
    } else if (cmd == PATH_DOWN) {
        svg_sz = 537.795;
    } else if (cmd == PATH_PINWHEEL) {
        svg_sz = 612;
    } else if (cmd == PATH_THERMOMETER) {
        svg_sz = 792;
    } else if (cmd == PATH_UMBRELLA) {
        svg_sz = 51.619;
    } else {
        svg_sz = PATH_SIZE;
    }

    pv = (double)size/2;
    base = (double)size/svg_sz;

    if (cmds) {
        int i = 0;
        for (i = 0 ; cmds[i] ; i++) {
            one = PATH_CREATE(group);
            nemoshow_item_path_cmd(one, cmds[i]);
            nemoshow_item_pivot(one, pv, pv);
            nemoshow_item_path_scale(one, base, base);
        }
    } else {
        one = PATH_CREATE(group);
        nemoshow_item_path_cmd(one, cmd);
        nemoshow_item_pivot(one, pv, pv);
        nemoshow_item_path_scale(one, base, base);
    }
    return group;
}

static void _weather_clear_trans(struct showone *sun, struct nemoshow *show)
{
    struct showone *one, *child;
    double base;
    one = ONE_CHILD0(sun);
    base = NEMOSHOW_ITEM_AT(one, sx);

    NemoMotion *m;
    m = nemomotion_create(sun->show, NEMOEASE_CUBIC_INOUT_TYPE, TRANS_TIME, 0);
    nemomotion_attach(m, 0.5,
            one, "sx", base * 1.25,
            one, "sy", base * 1.25, NULL);
    nemomotion_attach(m, 1.0,
            one, "sx", base,
            one, "sy", base, NULL);
    nemomotion_run(m);

    nemoshow_children_for_each(child, sun) {
        base = NEMOSHOW_ITEM_AT(child, sx);
        double ro = NEMOSHOW_ITEM_AT(child, ro);
        ro += 180;
        if (ro == 360) ro = 0;

        NemoMotion *m;
        m = nemomotion_create(sun->show, NEMOEASE_CUBIC_INOUT_TYPE, TRANS_TIME, 250);
        nemomotion_attach(m, 0.5,
                child, "sx", base * 1.15,
                child, "sy", base * 1.15, NULL);
        nemomotion_attach(m, 1.0,
                child, "sx", base,
                child, "sy", base,
                child, "ro", ro, NULL);
        nemomotion_run(m);
    }
}

static struct showone *_weather_clear(struct nemoshow *show, struct showone *parent, int x, int y, int size)
{
    struct showone *sun, *child;
    sun = _path_create(show, parent, size, NULL, PATH_SUN);
    _path_set_fill(sun, COLOR_BASE, 0.0);
    nemoshow_item_translate(sun, x, y);

    nemoshow_children_for_each(child, sun) {
        _nemoshow_item_motion(child, NEMOEASE_CUBIC_INOUT_TYPE, TRANS_TIME, 0,
                "alpha", 1.0f,
                NULL);
    }
    _weather_clear_trans(sun, show);

    return sun;
}

static void _weather_clear_night_trans(struct showone *moon, struct nemoshow *show)
{
    struct showone *one;
    double base;
    one = ONE_CHILD0(moon);
    base = NEMOSHOW_ITEM_AT(one, sx);

    NemoMotion *m;
    m = nemomotion_create(moon->show, NEMOEASE_CUBIC_OUT_TYPE, TRANS_TIME, 0);
    nemomotion_attach(m, 0.50,
            one, "sx", base * 1.15,
            one, "sy", base * 1.15, NULL);
    nemomotion_attach(m, 0.75,
            one, "sx", base * 0.90,
            one, "sy", base * 0.90, NULL);
    nemomotion_attach(m, 1.00,
            one, "sx", base * 1.00,
            one, "sy", base * 1.00, NULL);
    nemomotion_run(m);
}

static struct showone *_weather_clear_night(struct nemoshow *show, struct showone *parent, int x, int y, int size)
{
    struct showone *moon, *one;

    moon = _path_create(show, parent, size, PATH_MOON, NULL);
    _path_set_fill(moon, COLOR_BASE, 0.0);
    nemoshow_item_translate(moon, x, y);

    one = ONE_CHILD0(moon);
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_OUT_TYPE, TRANS_TIME, 0,
            "alpha", 1.0f,
            NULL);
    _weather_clear_night_trans(moon, show);
    return moon;
}

static void _weather_fog_trans(struct showone *group, struct nemoshow *show)
{
    struct showone *one;
    double base;
    one = ONE_CHILD0(group);
    base = NEMOSHOW_ITEM_AT(one, sx);
    NemoMotion *m;
    m = nemomotion_create(group->show, NEMOEASE_CUBIC_OUT_TYPE, TRANS_TIME, 0);
    nemomotion_attach(m, 0.25, one, "sy", base * 1.15, NULL);
    nemomotion_attach(m, 0.50, one, "sy", base * 0.90, NULL);
    nemomotion_attach(m, 0.75, one, "sy", base * 1.15, NULL);
    nemomotion_attach(m, 1.00, one, "sy", base * 1.00, NULL);
    nemomotion_run(m);
}

static struct showone *_weather_fog(struct nemoshow *show, struct showone *parent,
        int x, int y, int size)
{
    struct showone *fog, *one;

    fog = _path_create(show, parent, size, PATH_FOG, NULL);
    _path_set_fill(fog, COLOR_BASE, 0.0);
    nemoshow_item_translate(fog, x, y);

    one = ONE_CHILD0(fog);
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_OUT_TYPE, TRANS_TIME, 0,
            "alpha", 1.0f,
            NULL);
    _weather_fog_trans(fog, show);

    return fog;
}

static void _weather_cloud_trans(struct showone *group, struct nemoshow *show)
{
    NemoMotion *m;
    m = nemomotion_create(group->show, NEMOEASE_LINEAR_TYPE, TRANS_TIME, 0);
    nemomotion_attach(m, 0.25, group, "sx", 1.1, NULL);
    nemomotion_attach(m, 0.50, group, "sx", 1.0, NULL);
    nemomotion_attach(m, 0.75, group, "sx", 1.1, NULL);
    nemomotion_attach(m, 1.00, group, "sx", 1.0, NULL);
    nemomotion_run(m);
}

static struct showone *_weather_cloud(struct nemoshow *show, struct showone *parent, int x, int y, int size)
{
    struct showone *cloud, *one;

    cloud = _path_create(show, parent, size, PATH_CLOUD, NULL);
    _path_set_fill(cloud, COLOR_BASE, 0.0);
    nemoshow_item_translate(cloud, x, y);

    one = ONE_CHILD0(cloud);
    _nemoshow_item_motion(one, NEMOEASE_LINEAR_TYPE, TRANS_TIME, 0,
            "alpha", 1.0f,
            NULL);

    _weather_cloud_trans(cloud, show);

    return cloud;
}

static void _weather_cloudy_trans(struct showone *group, struct nemoshow *show, double tx)
{
    _nemoshow_item_motion(group, NEMOEASE_CUBIC_INOUT_TYPE, TRANS_TIME, 0,
            "tx", tx,
            NULL);
}


static struct showone *_weather_cloudy(struct nemoshow *show, struct showone *parent, int x, int y, int size)
{
    struct showone *group, *cloudy, *one;

    group = GROUP_CREATE(parent);
    nemoshow_item_translate(group, x, y);

    cloudy = _path_create(show, group, size/2.0, PATH_CLOUD2, NULL);
    _path_set_fill(cloudy, COLOR_BASE, 0.0);
    nemoshow_item_translate(cloudy, 300, 0);

    one = ONE_CHILD0(cloudy);
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_IN_TYPE, TRANS_TIME, 0,
            "alpha", 1.0f,
            NULL);
    _weather_cloudy_trans(cloudy, show, -50);

    cloudy = _path_create(show, group, size/1.5, PATH_CLOUD2, NULL);
    _path_set_fill(cloudy, COLOR_BASE, 0.0);
    nemoshow_item_translate(cloudy, -150, 200);

    one = ONE_CHILD0(cloudy);
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_IN_TYPE, TRANS_TIME, 0,
            "alpha", 1.0f,
            NULL);
    _weather_cloudy_trans(cloudy, show, 250);

    return group;
}

static struct showone *_weather_rain(struct nemoshow *show, struct showone *parent,
        int x, int y, int _size, int cnt, bool drizzle)
{
    struct showone *group, *rain, *one;

    const char *cmd;
    if (drizzle) cmd = PATH_DRIZZLE;
    else cmd = PATH_RAIN;

    group = GROUP_CREATE(parent);
    nemoshow_item_translate(group, x, y);

    int i = 0;
    double size = _size/8.0;

    int diff = (double)(_size - 100)/cnt;
    int adv = 75;
    double ty = 0;


    for (i = 0 ; i < cnt ; i++) {
        rain = _path_create(show, group, size, cmd, NULL);
        _path_set_fill(rain, COLOR_BASE, 0.0);
        nemoshow_item_translate(rain, adv, _size - 200);
        nemoshow_item_rotate(rain, 45.0);
        nemoshow_item_set_alpha(ONE_CHILD0(rain), 0.0);
        one = ONE_CHILD0(rain);

        if (i%2) ty = _size - 50;
        else     ty = _size - 75;

        _nemoshow_item_motion(rain, NEMOEASE_CUBIC_INOUT_TYPE, TRANS_TIME, i * 250,
                "ty", ty,
                "ro", 0.0f,
                NULL);

        _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, TRANS_TIME, i * 250,
                "alpha", 1.0f,
                NULL);

        adv += diff;
    }

    return group;
}

static struct showone *_weather_snow(struct nemoshow *show, struct showone *parent,
        int x, int y, int _size, int cnt, bool sleet)
{
    struct showone *group, *snow, *one;

    group = GROUP_CREATE(parent);
    nemoshow_item_translate(group, x, y);

    int i = 0;
    double size = _size/8.0;

    int diff = (double)(_size - 100)/cnt;
    int adv = 50;

    double ty = 0;
    for (i = 0 ; i < cnt ; i++) {
        snow = _path_create(show, group, size, PATH_SNOW, NULL);
        if (sleet) _path_set_stroke(snow, COLOR_BASE, 0.0, 5);
        else _path_set_fill(snow, COLOR_BASE, 0.0);
        nemoshow_item_translate(snow, adv, _size - 200);
        nemoshow_item_rotate(snow, 0);

        one = ONE_CHILD0(snow);
        nemoshow_item_set_alpha(one, 0.0);

        if (i%2) ty = _size - 100;
        else     ty = _size;
        _nemoshow_item_motion(snow, NEMOEASE_CUBIC_INOUT_TYPE, TRANS_TIME, i * 250,
                "ty", ty,
                NULL);
        _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, TRANS_TIME, i * 250,
                "alpha", 1.0f,
                "ro",    45.0,
                NULL);
        adv += diff;
    }

    return group;
}

static struct showone *_weather_thunder(struct nemoshow *show, struct showone *parent,
        int x, int y, int _size, int cnt)
{
    struct showone *group, *thunder, *one;

    group = GROUP_CREATE(parent);
    nemoshow_item_translate(group, x, y);

    int i = 0;
    double size = _size/8.0;

    int diff = (double)(_size - 100)/cnt;
    int adv = 50;

    for (i = 0 ; i < cnt ; i++) {
        int duration;
        if (i%2) duration = 1000;
        else duration = 0;
        thunder = _path_create(show, group, size, PATH_THUNDER, NULL);
        _path_set_fill(thunder, COLOR_BASE, 0);
        nemoshow_item_translate(thunder, adv, _size - i * 30);
        one = ONE_CHILD0(thunder);

        double base;
        base = NEMOSHOW_ITEM_AT(one, sy);
        NemoMotion *m;
        m = nemomotion_create(one->show, NEMOEASE_CUBIC_INOUT_TYPE, TRANS_TIME, duration);
        nemomotion_attach(m, 0.25, one, "alpha", 1.0f, NULL);
        nemomotion_attach(m, 0.50, one, "alpha", 0.0f, NULL);
        nemomotion_attach(m, 0.75, one, "alpha", 1.0f, NULL);
        nemomotion_attach(m, 0.85, one, "alpha", 0.0f, NULL);
        nemomotion_attach(m, 1.00, one, "alpha", 1.0f, NULL);
        nemomotion_attach(m, 1.00, one, "sy", 2.5 * base, NULL);

        adv += diff;
    }

    return group;
}

static struct showone *_weather_wind(struct nemoshow *show, struct showone *parent,
        int x, int y, int size)
{
    struct showone *group, *wind, *one0, *one1, *one2;

    group = GROUP_CREATE(parent);
    nemoshow_item_translate(group, x, y);

    wind = _path_create(show, group, size, NULL, PATH_WIND);
    _path_set_fill(wind, COLOR_BASE, 0.0);
    one0 = ONE_CHILD0(wind);
    one1 = ONE_CHILD1(wind);
    one2 = ONE_CHILD2(wind);

    nemoshow_item_set_alpha(one0, 0.0);
    nemoshow_item_set_alpha(one1, 0.0);
    nemoshow_item_set_alpha(one2, 0.0);
    double tx0, tx1, tx2;
    tx0 = NEMOSHOW_ITEM_AT(one0, tx);
    tx1 = NEMOSHOW_ITEM_AT(one1, tx);
    tx2 = NEMOSHOW_ITEM_AT(one2, tx);
    nemoshow_item_translate(one0,
            tx0 - 300, NEMOSHOW_ITEM_AT(one0, ty));
    nemoshow_item_translate(one1,
            tx1 - 300, NEMOSHOW_ITEM_AT(one1, ty));
    nemoshow_item_translate(one2,
            tx2 - 300, NEMOSHOW_ITEM_AT(one2, ty));

    _nemoshow_item_motion(one0, NEMOEASE_CUBIC_IN_TYPE, TRANS_TIME, 0,
            "alpha", 1.0,
            "tx",    tx0,
            NULL);
    _nemoshow_item_motion(one1, NEMOEASE_CUBIC_IN_TYPE, TRANS_TIME, 500,
            "alpha", 1.0,
            "tx",    tx1,
            NULL);
    _nemoshow_item_motion(one2, NEMOEASE_CUBIC_IN_TYPE, TRANS_TIME, 1000,
            "alpha", 1.0,
            "tx",    tx2,
            NULL);

    return group;
}

static void _weather_now_hide_done_free(struct nemotimer *timer, void *userdata)
{
    Weather *weather = userdata;
    nemoshow_item_destroy(weather->now.group_old);
    weather->now.group_old = NULL;
}

static void _weather_now_hide(Weather *weather)
{
    double dur, delay;
    struct showone *one, *one0, *one1, *one2;

    if (!weather->now.weather) return;

    if (weather->now.timer) {
        nemotimer_destroy(weather->now.timer);
        weather->now.timer = NULL;
    }

    dur = 1000;
    delay = 100;

    // Normal temperature
    one = weather->now.temp;
    one0 = weather->now.temp0;
    one1 = weather->now.temp1;
    one2 = weather->now.temp_minus;

    _nemoshow_item_motion(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", -100.0f,
            "alpha", 0.0f,
            NULL);

    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx",    -100.0f,
            "alpha", 0.0f,
            NULL);

    _nemoshow_item_motion(one1, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", -100.0f,
            "alpha", 0.0,
            NULL);

    _nemoshow_item_motion(ONE_CHILD0(one1), NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            NULL);

    _nemoshow_item_motion(one2, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx",    -100.0f,
            "alpha", 0.0f,
            NULL);

    // City, country
    delay = 100;
    one = weather->now.city_country;
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx",    1100.0f,
            NULL);

    // Main description
    delay = 100;
    one = weather->now.main;
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx",    -100.0f,
            NULL);

    double x = -160;
    // Rain/Cloud/Snow
    delay = 100;
    _nemoshow_item_motion(weather->now.rsc, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x,
            NULL);
    _nemoshow_item_motion(weather->now.rsc_in, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x,
            NULL);
    _nemoshow_item_motion(weather->now.rsc_clip, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x,
            NULL);

    one = weather->now.rsc0;
    one0 = weather->now.rsc1;
    one1 = weather->now.rsc2;
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx",    x,
            NULL);

    _nemoshow_item_motion(one1, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx",    x,
            NULL);

    _nemoshow_item_motion(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx",    x,
            NULL);

    // Pressure
    delay = 600;
    one = weather->now.pressure;
    _nemoshow_item_motion(ONE_CHILD0(one), NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            NULL);
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x,
            NULL);
    _nemoshow_item_motion(weather->now.pressure_in, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x,
            NULL);
    _nemoshow_item_motion(weather->now.pressure_clip, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x,
            NULL);

    one = weather->now.pressure0;
    one0 = weather->now.pressure1;
    one1 = weather->now.pressure2;
    one2 = weather->now.pressure3;

    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx",    x,
            NULL);
    _nemoshow_item_motion(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx",    x,
            NULL);
    _nemoshow_item_motion(one1, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx",    x,
            NULL);
    _nemoshow_item_motion(one2, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx",    x,
            NULL);

    x = 1024 + 150;
    // Wind
    delay = 600;
    one = weather->now.wind;
    _nemoshow_item_motion(ONE_CHILD0(one), NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 1.0f,
            NULL);
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x,
            NULL);

    one = weather->now.wind0;
    one0 = weather->now.wind1;
    one1 = weather->now.wind2;
    one2 = weather->now.wind3;

    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx", x,
            NULL);

    _nemoshow_item_motion(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx",    x,
            NULL);

    _nemoshow_item_motion(one1, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx",    x,
            NULL);

    _nemoshow_item_motion(one2, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx",    x,
            NULL);

    // Humidity
    delay = 100;
    one = weather->now.humidity;
    _nemoshow_item_motion(ONE_CHILD0(one), NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            NULL);
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x,
            NULL);
    _nemoshow_item_motion(weather->now.humidity_in, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x,
            NULL);
    _nemoshow_item_motion(weather->now.humidity_clip, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x,
            NULL);

    one = weather->now.humidity0;
    one0 = weather->now.humidity1;
    one1 = weather->now.humidity2;

    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx",    x,
            NULL);
    _nemoshow_item_motion(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx",    x,
            NULL);
    _nemoshow_item_motion(one1, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx",    x,
            NULL);

    // Main weather icon
    delay = 600;
    if (weather->now.weather) {
        one = weather->now.weather;
        struct showone *child;
        nemoshow_children_for_each(child, one) {
            _nemoshow_item_motion(child, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
                    "alpha", 0.0f,
                    "sx",    0.0f,
                    "sy",    0.0f,
                    NULL);
        }
    }
    if (weather->now.weather0) {
        one = weather->now.weather0;
        struct showone *child;
        nemoshow_children_for_each(child, one) {
            _nemoshow_item_motion(child, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
                    "alpha", 0.0f,
                    "sx",    0.0f,
                    "sy",    0.0f,
                    NULL);
        }
    }
    if (weather->now.weather1) {
        one = weather->now.weather1;
        struct showone *child;
        nemoshow_children_for_each(child, one) {
            _nemoshow_item_motion(child, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
                    "alpha", 0.0f,
                    "sx",    0.0f,
                    "sy",    0.0f,
                    NULL);
        }
    }

    // FIXME: destory itself is not possible
    struct nemotimer *timer;
    timer = nemotimer_create(weather->tool);
    nemotimer_set_timeout(timer, dur + delay);
    nemotimer_set_callback(timer, _weather_now_hide_done_free);
    nemotimer_set_userdata(timer, weather);

    weather->now.group_old = weather->now.group;
    nemoshow_dispatch_frame(weather->show);
}

static void _weather_now_timeout(struct nemotimer *timer, void *userdata)
{
    Weather *weather = userdata;
    struct nemoshow *show = weather->show;
    struct showone *group = weather->now.group;
    struct showone *one = NULL, *one0 = NULL, *one1 = NULL;

    int x, y, size;
    x = 400;
    y = 70;
    size = 512;
    WeatherNow *w = weather->now.w;
    int id = w->weather_id;
    if (id == 800) {    // Clear Sky
        one =  weather->now.weather;
        if (w->datetime > w->sunset)
            _weather_clear_night_trans(weather->now.weather, show);
        else
            _weather_clear_trans(weather->now.weather, show);
    } else if (id == 801 || id == 802) {    // Cloud
        if (w->datetime > w->sunset)
            _weather_clear_night_trans(weather->now.weather, show);
        else
            _weather_clear_trans(weather->now.weather, show);
        _weather_cloud_trans(weather->now.weather0, show);
    } else if (id == 803 || id == 804) {    // More Cloud
        _weather_cloud_trans(weather->now.weather, show);
        if (NEMOSHOW_ITEM_AT(ONE_CHILD0(weather->now.weather0), tx) == 300) {
            _weather_cloudy_trans(ONE_CHILD0(weather->now.weather0), show, -50);
            _weather_cloudy_trans(ONE_CHILD1(weather->now.weather0), show, 300);
        } else {
            _weather_cloudy_trans(ONE_CHILD0(weather->now.weather0), show, 300);
            _weather_cloudy_trans(ONE_CHILD1(weather->now.weather0), show, -150);
        }
    } else if (200 <= id && id < 300) { // Thunderstorm
        _weather_cloud_trans(weather->now.weather, show);
        if (weather->now.weather0) nemoshow_item_destroy(weather->now.weather0);
        if (weather->now.weather1) nemoshow_item_destroy(weather->now.weather1);

        if (id == 211)
            one0 = _weather_thunder(show, group, x, y, size, 5);
        else if (id == 212 || id == 221)
            one0 = _weather_thunder(show, group, x, y, size, 7);
        else one0 = _weather_thunder(show, group, x, y, size, 3);

        if (id == 200 || id == 230)
            one1 = _weather_rain(show, group, x, y, size, 3, true);
        else if (id == 201 || id == 231)
            one1 = _weather_rain(show, group, x, y, size, 4, true);
        else if (id == 202 || id == 232)
            one1 = _weather_rain(show, group, x, y, size, 6, true);

        weather->now.weather0 = one0;
        weather->now.weather1 = one1;
    } else if (300 <= id && id < 400) {
        _weather_cloud_trans(weather->now.weather, show);
        if (weather->now.weather0) nemoshow_item_destroy(weather->now.weather0);

        if (id >= 321)
            one0 = _weather_rain(show, group, x, y, size, 9, true);
        else if ((id % 10) == 1 || (id % 10) == 0)
            one0 = _weather_rain(show, group, x, y, size, 3, true);
        else if ((id % 10) == 2 || (id % 10) == 2)
            one0 = _weather_rain(show, group, x, y, size, 5, true);
        else
            one0 = _weather_rain(show, group, x, y, size, 7, true);

        weather->now.weather0 = one0;
    } else if (500 <= id && id < 600) {
        _weather_cloud_trans(weather->now.weather, show);
        if (weather->now.weather0) nemoshow_item_destroy(weather->now.weather0);

        if (id >= 521)
            one0 = _weather_rain(show, group, x, y, size, 9, true);
        else if ((id % 10) == 1 || (id % 10) == 0)
            one0 = _weather_rain(show, group, x, y, size, 3, true);
        else if ((id % 10) == 2 || (id % 10) == 2)
            one0 = _weather_rain(show, group, x, y, size, 5, true);
        else
            one0 = _weather_rain(show, group, x, y, size, 7, true);

        weather->now.weather0 = one0;
    } else if (600 <= id && id < 700) {
        _weather_cloud_trans(weather->now.weather, show);
        if (weather->now.weather0) nemoshow_item_destroy(weather->now.weather0);

        if (id >= 621) {
            one0 = _weather_snow(show, group, x, y, size, 9, false);
        } else if (id == 611) {
            one0 = _weather_snow(show, group, x, y, size, 3, true);
        } else if (id == 612) {
            one0 = _weather_snow(show, group, x, y, size, 6, true);
        } else if (id == 615) {
            one0 = _weather_snow(show, group, x, y, size, 4, false);
            one0 = _weather_rain(show, group, x, y, size, 3, false);
        } else if ((id % 10) == 1 || (id % 10) == 0) {
            one0 = _weather_snow(show, group, x, y, size, 3, false);
        } else if ((id % 10) == 2 || (id % 10) == 2) {
            one0 = _weather_snow(show, group, x, y, size, 5, false);
        } else {
            one0 = _weather_snow(show, group, x, y, size, 7, false);
        }

        weather->now.weather0 = one0;
    } else if (700 <= id && id < 800) {
        _weather_fog_trans(weather->now.weather, show);
    } else if (id == 900 || id == 901 || id == 902 ||
            id == 905 || id == 906 ||
            (952 <= id && id <= 962)) {

        if (weather->now.weather) nemoshow_item_destroy(weather->now.weather);
        one = _weather_wind(show, group, x, y, size);
        weather->now.weather = one;

    } else if (id == 903 || id == 904) {
        if (w->datetime > w->sunset)
            _weather_clear_night_trans(weather->now.weather, show);
        else
            _weather_clear_trans(weather->now.weather, show);
    } else {
        ERR("Something wrong!!!");
    }

    double ty, ty0;
    ty = NEMOSHOW_ITEM_AT(weather->now.rsc, ty);
    ty0 = NEMOSHOW_ITEM_AT(weather->now.rsc_clip, ty);
    NemoMotion *m;
    m = nemomotion_create(show,  NEMOEASE_CUBIC_INOUT_TYPE, TRANS_TIME, 1000);
    nemomotion_attach(m, 0.25, weather->now.rsc_clip, "ty", ty + 150 * 1.0, NULL);
    nemomotion_attach(m, 0.50, weather->now.rsc_clip, "ty", ty, NULL);
    nemomotion_attach(m, 1.00, weather->now.rsc_clip, "ty", ty0, NULL);
    nemomotion_run(m);

    ty = NEMOSHOW_ITEM_AT(weather->now.pressure, ty);
    ty0 = NEMOSHOW_ITEM_AT(weather->now.pressure_clip, ty);
    m = nemomotion_create(show, NEMOEASE_CUBIC_INOUT_TYPE, TRANS_TIME, 1500);
    nemomotion_attach(m, 0.25, weather->now.pressure_clip, "ty", ty + 150 * 1.0, NULL);
    nemomotion_attach(m, 0.50, weather->now.pressure_clip, "ty", ty, NULL);
    nemomotion_attach(m, 1.00, weather->now.pressure_clip, "ty", ty0, NULL);
    nemomotion_run(m);

    one = weather->now.wind;
    double ro = 0;
    if (NEMOSHOW_ITEM_AT(ONE_CHILD0(one), ro) < 0)
        ro = 360.0 * w->wind_speed;
    else
        ro = -360.0 * w->wind_speed;

    _nemoshow_item_motion(ONE_CHILD0(one), NEMOEASE_CUBIC_INOUT_TYPE, TRANS_TIME, 2000,
            "ro", ro,
            NULL);

    ty = NEMOSHOW_ITEM_AT(weather->now.humidity_in, ty);
    ty0 = NEMOSHOW_ITEM_AT(weather->now.humidity_clip, ty);
    m = nemomotion_create(show, NEMOEASE_CUBIC_INOUT_TYPE, TRANS_TIME, 2500);
    nemomotion_attach(m, 0.25, weather->now.humidity_clip, "ty", ty + 150 * 1.0, NULL);
    nemomotion_attach(m, 0.50, weather->now.humidity_clip, "ty", ty, NULL);
    nemomotion_attach(m, 1.00, weather->now.humidity_clip, "ty", ty0, NULL);
    nemomotion_run(m);

    nemotimer_set_timeout(weather->now.timer, NOW_TIME);
    nemoshow_dispatch_frame(weather->show);
}

static void _weather_daily_hide(Weather *weather)
{
    double dur, delay;
    struct showone *one, *one0, *one1, *one2, *one3;

    if (!weather->daily.temp_max) return;

    dur = 1000;
    delay = 100;
    // Max temperature
    one = weather->daily.temp_max;
    one0 = weather->daily.temp_max0;
    one1 = weather->daily.temp_max1;
    one2 = weather->daily.temp_max2;
    one3 = weather->daily.temp_max_minus;

    _nemoshow_item_motion(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx",    -100.0f,
            NULL);
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", -100.0f,
            NULL);
    _nemoshow_item_motion(one1, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx",    -100.0f,
            "alpha", 0.0f,
            NULL);
    _nemoshow_item_motion(one2, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", -100.0f,
            NULL);
    _nemoshow_item_motion(ONE_CHILD0(one2), NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            NULL);
    _nemoshow_item_motion(one3, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", -100.0f,
            "alpha", 0.0f,
            NULL);

    // Min temperature
    one = weather->daily.temp_min;
    one0 = weather->daily.temp_min0;
    one1 = weather->daily.temp_min1;
    one2 = weather->daily.temp_min2;
    one3 = weather->daily.temp_min_minus;

    _nemoshow_item_motion(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx",    -100.0f,
            NULL);
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx",  -100.0f,
            NULL);
    _nemoshow_item_motion(one1, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            "tx",    -100.0f,
            NULL);
    _nemoshow_item_motion(one2, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", -100.0f,
            NULL);
    _nemoshow_item_motion(ONE_CHILD0(one2), NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 0.0f,
            NULL);
    _nemoshow_item_motion(one3, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", -100.0f,
            "alpha", 0.0f,
            NULL);
    nemoshow_dispatch_frame(weather->show);
}

static void _weather_daily_show(Weather *weather)
{
    WeatherDaily *w = weather->daily.w;
    struct nemoshow *show = weather->show;
    struct showone *group;

    group = GROUP_CREATE(weather->group);
    weather->daily.group = group;

    double x = 24;
    double y = 24;
    double textsize;
    int amount;
    double dur = 1000;
    double delay;

    struct showone *one, *one0, *one1, *one2, *one3;
    WeatherDay *ww = LIST_DATA(LIST_FIRST(w->days));

    // Max temperature
    y = 260;
    textsize = 35;
    delay = 250;
    one = _path_create(show, group, textsize - 10, PATH_DOWN, NULL);
    _path_set_fill(one, COLOR_TEXT, 0.0);
    nemoshow_item_translate(one, -100, y);
    nemoshow_item_rotate(ONE_CHILD0(one), 180);
    one0 = NUM_CREATE(group, COLOR_TEXT, 5, textsize, 0);
    nemoshow_item_set_alpha(one0, 0.0);
    nemoshow_item_translate(one0, -100, y);

    one1 = NUM_CREATE(group, COLOR_TEXT, 5, textsize, 0);
    nemoshow_item_set_alpha(one1, 0.0);
    nemoshow_item_translate(one1, -100, y);

    one2 = _path_create(show, group, textsize + 25, PATH_CELCIUS, NULL);
    _path_set_fill(one2, COLOR_TEXT, 0.0);
    nemoshow_item_translate(one2, -100, y - 15);

    one3 =  TEXT_CREATE(group, weather->font, 25, "");
    nemoshow_item_set_fill_color(one3, RGBA(COLOR_TEXT));
    nemoshow_item_translate(one3, -100, y + 5);
    nemoshow_item_set_alpha(one3, 0.0);
    if (ww->temp_max < 0) {
        nemoshow_item_set_text(one3, "-");
    }

    weather->daily.temp_max = one;
    weather->daily.temp_max0 = one0;
    weather->daily.temp_max1 = one1;
    weather->daily.temp_max2 = one2;
    weather->daily.temp_max_minus = one3;

    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x + 20,
            NULL);

    _nemoshow_item_motion(ONE_CHILD0(one), NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 1.0f,
            NULL);

    _nemoshow_item_motion(one1, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 1.0f,
            "tx",    x + 90,
            NULL);

    _nemoshow_item_motion(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 1.0f,
            "tx",    x + 60,
            NULL);

    _nemoshow_item_motion(one2, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x + 120,
            NULL);
    _nemoshow_item_motion(ONE_CHILD0(one2), NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 1.0f,
            NULL);

    _nemoshow_item_motion(one3, NEMOEASE_CUBIC_INOUT_TYPE, 1000, delay,
            "alpha", 1.0,
            "tx", x + 50,
            NULL);

    amount = ww->temp_max;
    int num0 = 0;
    int num1 = 0;
    if (amount >= 99) {
        num0 = 9;
        num1 = 9;
    } else if (amount < 0) {
        num0 = -amount/10;
        num1 = -amount%10;
    } else {
        num0 = amount/10;
        num1 = amount%10;

    }

    if (num0 == 0) {
        _nemoshow_item_motion(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 1000,
                "alpha", 0.0,
                NULL);
    } else {
        _nemoshow_item_motion(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 1000,
                "alpha", 1.0,
                NULL);
    }
    NUM_UPDATE(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 1000, num0);
    NUM_UPDATE(one1, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 1000, num1);

    // Min temperature
    y += 60;
    delay = 500;
    one = _path_create(show, group, textsize - 10, PATH_DOWN, NULL);
    _path_set_fill(one, COLOR_TEXT, 0.0);
    nemoshow_item_translate(one, -100, y + 10);
    one0 = NUM_CREATE(group, COLOR_TEXT, 5, textsize, 0);
    nemoshow_item_set_alpha(one0, 0.0);
    nemoshow_item_translate(one0, -100, y);

    one1 = NUM_CREATE(group, COLOR_TEXT, 5, textsize, 0);
    nemoshow_item_set_alpha(one1, 0.0);
    nemoshow_item_translate(one1, -100, y);

    one2 = _path_create(show, group, textsize + 25, PATH_CELCIUS, NULL);
    _path_set_fill(one2, COLOR_TEXT, 0.0);
    nemoshow_item_translate(one2, -100, y - 15);

    one3 =  TEXT_CREATE(group, weather->font, 25, "");
    nemoshow_item_set_fill_color(one3, RGBA(COLOR_TEXT));
    nemoshow_item_translate(one3, -100, y + 5);
    nemoshow_item_set_alpha(one3, 0.0);
    if (ww->temp_min < 0) {
        nemoshow_item_set_text(one3, "-");
    }

    weather->daily.temp_min = one;
    weather->daily.temp_min0 = one0;
    weather->daily.temp_min1 = one1;
    weather->daily.temp_min2 = one2;
    weather->daily.temp_min_minus = one3;

    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 1000, delay,
            "tx", x + 20,
            NULL);

    _nemoshow_item_motion(ONE_CHILD0(one), NEMOEASE_CUBIC_INOUT_TYPE, 1000, delay,
            "alpha", 1.0f,
            NULL);

    _nemoshow_item_motion(one0, NEMOEASE_CUBIC_INOUT_TYPE, 1000, delay,
            "alpha", 1.0f,
            "tx",    x + 60,
            NULL);

    _nemoshow_item_motion(one1, NEMOEASE_CUBIC_INOUT_TYPE, 1000, delay,
            "alpha", 1.0f,
            "tx",    x + 90,
            NULL);

    _nemoshow_item_motion(one2, NEMOEASE_CUBIC_INOUT_TYPE, 1000, delay,
            "tx", x + 120,
            NULL);
    _nemoshow_item_motion(ONE_CHILD0(one2), NEMOEASE_CUBIC_INOUT_TYPE, 1000, delay,
            "alpha", 1.0f,
            NULL);

    _nemoshow_item_motion(one3, NEMOEASE_CUBIC_INOUT_TYPE, 1000, delay,
            "alpha", 1.0,
            "tx", x + 50,
            NULL);

    amount = ww->temp_min;
    num0 = 0;
    num1 = 0;
    if (amount >= 99) {
        num0 = 9;
        num1 = 9;
    } else if (amount < 0) {
        num0 = -amount/10;
        num1 = -amount%10;
    } else {
        num0 = amount/10;
        num1 = amount%10;

    }

    if (num0 == 0) {
        _nemoshow_item_motion(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 1000,
                "alpha", 0.0,
                NULL);
    } else {
        _nemoshow_item_motion(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 1000,
                "alpha", 1.0,
                NULL);
    }
    NUM_UPDATE(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 1000, num0);
    NUM_UPDATE(one1, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 1000, num1);
    nemoshow_dispatch_frame(weather->show);
}

static void _weather_now_show(Weather *weather)
{
    WeatherNow *w = weather->now.w;
    struct nemoshow *show = weather->show;
    struct showone *group;

    if (!weather->now.timer) {
        struct nemotimer *timer;
        timer = nemotimer_create(weather->tool);
        nemotimer_set_timeout(timer, NOW_TIME);
        nemotimer_set_callback(timer, _weather_now_timeout);
        nemotimer_set_userdata(timer, weather);
        weather->now.timer = timer;
    }

    group = GROUP_CREATE(weather->group);
    weather->now.group = group;

    int id = 803; //w->weather_id;

    int amount;
    double x = 24;
    double y = 24 + 30;
    int textsize = 0;
    int size = 0;
    double dur = 1000;
    double delay = 0;
    struct showone *one, *one0, *one1, *one2;

    // Temperature
    textsize = 150;
    one = NUM_CREATE(group, COLOR_BASE, 5, textsize, 0);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, -100, y);

    one0 = NUM_CREATE(group, COLOR_BASE, 5, textsize, 0);
    nemoshow_item_set_alpha(one0, 0.0);
    nemoshow_item_translate(one0, -100, y);

    one1 = _path_create(show, group, textsize + 25, PATH_CELCIUS, NULL);
    _path_set_fill(one1, COLOR_BASE, 1.0);
    nemoshow_item_translate(one1, -100, y - 40);

    one2 =  TEXT_CREATE(group, weather->font, 60, "");
    nemoshow_item_set_fill_color(one2, RGBA(COLOR_BASE));
    nemoshow_item_translate(one2, -100, y + 50);
    nemoshow_item_set_alpha(one2, 0.0);
    if (w->temp < 0) {
        nemoshow_item_set_text(one2, "-");
    }

    weather->now.temp = one;
    weather->now.temp0 = one0;
    weather->now.temp1 = one1;
    weather->now.temp_minus = one2;

    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "alpha", 1.0f,
            NULL);

    _nemoshow_item_motion(ONE_CHILD0(one1), NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "alpha", 1.0f,
            NULL);

    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "tx", x,
            NULL);

    _nemoshow_item_motion(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "alpha", 1.0f,
            "tx",    x + 105.0f,
            NULL);
    _nemoshow_item_motion(one1, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "tx", x + 200.0f,
            NULL);

    _nemoshow_item_motion(one2, NEMOEASE_CUBIC_INOUT_TYPE, 1000, delay,
            "alpha", 1.0,
            "tx", x,
            NULL);

    amount = w->temp;
    int num0 = 0;
    int num1 = 0;
    if (amount >= 99) {
        num0 = 9;
        num1 = 9;
    } else if (amount < 0) {
        num0 = -amount/10;
        num1 = -amount%10;
    } else {
        num0 = amount/10;
        num1 = amount%10;
    }
    if (num0 == 0) {
        _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 1000,
                "alpha", 0.0,
                NULL);
    } else {
        _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 1000,
                "alpha", 1.0,
                NULL);
    }
    NUM_UPDATE(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 1000, num0);
    NUM_UPDATE(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 1000, num1);

    // City, country
    x = 960;
    char buf[PATH_MAX];
    snprintf(buf, PATH_MAX, "%s,%s",
            w->city, w->country);
    one = TEXT_CREATE(group, weather->font, 50, buf);
    nemoshow_item_set_fill_color(one, RGBA(COLOR_TEXT));
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, x + 100, y);
    nemoshow_item_set_anchor(one, 1.0, -0.5);

    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 500,
            "alpha", 1.0f,
            "tx",    x,
            NULL);
    weather->now.city_country = one;

    // Main description
    x = 24 + 30;
    y = 650;
    if (w->weather_desc) {
        one = TEXT_CREATE(group, weather->font, 70, w->weather_desc);
        nemoshow_item_set_fill_color(one, RGBA(COLOR_TEXT));
        nemoshow_item_set_alpha(one, 0.0);
        nemoshow_item_translate(one, -100, y);

        _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, 1000, 500,
                "alpha", 1.0f,
                "tx",    x,
                NULL);
        weather->now.main = one;
    }

    // Main Weather status ICON
    x = 400;
    y = 70;
    size = 512;


    one = NULL;
    one0 = NULL;
    one1 = NULL;
    if (id == 800) {    // Clear Sky
        if (w->datetime > w->sunset)
            one = _weather_clear_night(show, group, x, y, size);
        else
            one = _weather_clear(show, group, x, y, size);
    } else if (id == 801 || id == 802) {    // Cloud
        if (w->datetime > w->sunset)
            one = _weather_clear_night(show, group, x - 50, y - 50, 512);
        else
            one = _weather_clear(show, group, x - 50, y - 50, 512);
        one0 = _weather_cloud(show, group, x, y, size);
    } else if (id == 803 || id == 804) {    // More Cloud
        one = _weather_cloud(show, group, x, y, size);
        one0 = _weather_cloudy(show, group, x, y, size);
    } else if (200 <= id && id < 300) { // Thunderstorm
        one = _weather_cloud(show, group, x, y, size);
        if (id == 211)
            one0 = _weather_thunder(show, group, x, y, size, 5);
        else if (id == 212 || id == 221)
            one0 = _weather_thunder(show, group, x, y, size, 7);
        else one0 = _weather_thunder(show, group, x, y, size, 3);

        if (id == 200 || id == 230)
            one1 = _weather_rain(show, group, x, y, size, 3, true);
        else if (id == 201 || id == 231)
            one1 = _weather_rain(show, group, x, y, size, 4, true);
        else if (id == 202 || id == 232)
            one1 = _weather_rain(show, group, x, y, size, 6, true);
    } else if (300 <= id && id < 400) {
        one = _weather_cloud(show, group, x, y, size);
        if (id >= 321)
            one0 = _weather_rain(show, group, x, y, size, 9, true);
        else if ((id % 10) == 1 || (id % 10) == 0)
            one0 = _weather_rain(show, group, x, y, size, 3, true);
        else if ((id % 10) == 2 || (id % 10) == 2)
            one0 = _weather_rain(show, group, x, y, size, 5, true);
        else
            one0 = _weather_rain(show, group, x, y, size, 7, true);
    } else if (500 <= id && id < 600) {
        one = _weather_cloud(show, group, x, y, size);
        if (id >= 521)
            one0 = _weather_rain(show, group, x, y, size, 9, true);
        else if ((id % 10) == 1 || (id % 10) == 0)
            one0 = _weather_rain(show, group, x, y, size, 3, true);
        else if ((id % 10) == 2 || (id % 10) == 2)
            one0 = _weather_rain(show, group, x, y, size, 5, true);
        else
            one0 = _weather_rain(show, group, x, y, size, 7, true);
    } else if (600 <= id && id < 700) {
        one = _weather_cloud(show, group, x, y, size);
        if (id >= 621) {
            one0 = _weather_snow(show, group, x, y, size, 9, false);
        } else if (id == 611) {
            one0 = _weather_snow(show, group, x, y, size, 3, true);
        } else if (id == 612) {
            one0 = _weather_snow(show, group, x, y, size, 6, true);
        } else if (id == 615) {
            one0 = _weather_snow(show, group, x, y, size, 4, false);
            one0 = _weather_rain(show, group, x, y, size, 3, false);
        } else if ((id % 10) == 1 || (id % 10) == 0) {
            one0 = _weather_snow(show, group, x, y, size, 3, false);
        } else if ((id % 10) == 2 || (id % 10) == 2) {
            one0 = _weather_snow(show, group, x, y, size, 5, false);
        } else {
            one0 = _weather_snow(show, group, x, y, size, 7, false);
        }
    } else if (700 <= id && id < 800) {
        one = _weather_fog(show, group, x, y, size);
    } else if (id == 900 || id == 901 || id == 902 ||
            id == 905 || id == 906 ||
            (952 <= id && id <= 962)) {
        one = _weather_wind(show, group, x, y, size);
    } else if (id == 903 || id == 904) {
        if (w->datetime > w->sunset)
            one = _weather_clear_night(show, group, x - 50, y - 50, 512);
        else
            one = _weather_clear(show, group, x - 50 , y - 50, 512);
    } else {
        one = _path_create(show, group, size, PATH_NA, NULL);
        _path_set_fill(one, COLOR_BASE, 1.0);
        nemoshow_item_translate(one, x, y);
        _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
                "alpha", 1.0f,
                NULL);
    }
    weather->now.weather = one;
    weather->now.weather0 = one0;
    weather->now.weather1 = one1;

    textsize = 40;
    size = 150;
    y = 750;

    // Rain or Snow or Cloud
    const char *path = NULL;
    if (w->snow > 0) {
        path = PATH_SNOW;
        amount = w->snow;
    } else if (w->rain > 0) {
        path = PATH_UMBRELLA;
        amount = w->rain;
    } else {
        path = PATH_CLOUD;
        amount = w->cloud;
    }

    delay = 500;
    x = 24 + 35;

    one = PATH_CREATE(group);
    nemoshow_item_set_fill_color(one, RGBA(COLOR_BASE));
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, -size, y);
    nemoshow_item_path_moveto(one, -5, -5);
    nemoshow_item_path_lineto(one, size + 5, 0);
    nemoshow_item_path_lineto(one, size + 5, size + 5);
    nemoshow_item_path_lineto(one, 0, size + 5);
    nemoshow_item_path_close(one);
    weather->now.rsc_clip = one;

    one = _path_create(show, group, size, path, NULL);
    _path_set_fill(one, COLOR_BASE, 1.0);
    nemoshow_item_translate(one, -size, y);
    weather->now.rsc_in = one;
    nemoshow_item_set_clip(one, weather->now.rsc_clip);

    one = _path_create(show, group, size, path, NULL);
    _path_set_stroke(one, COLOR_BASE, 1.0, 5);
    nemoshow_item_translate(one, -size, y);
    weather->now.rsc = one;

    _nemoshow_item_motion(weather->now.rsc, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x,
            NULL);
    _nemoshow_item_motion(weather->now.rsc_in, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x,
            NULL);
    _nemoshow_item_motion(weather->now.rsc_clip, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x,
            NULL);

    NemoMotion *m;
    m = nemomotion_create(show, NEMOEASE_CUBIC_OUT_TYPE, dur * 5, delay + 1000);
    nemomotion_attach(m, 0.3, weather->now.rsc_clip, "ty", y + size * 1.0, NULL);
    nemomotion_attach(m, 0.7, weather->now.rsc_clip, "ty", y + size * 0.0, NULL);
    nemomotion_attach(m, 1.0, weather->now.rsc_clip, "ty", y + size - size * ((double)amount/100), NULL);
    nemomotion_run(m);

            /*
    _nemoshow_item_motion(weather->now.rsc_clip, NEMOEASE_CUBIC_OUT_TYPE, dur * 5, delay + 1000,
            "ty", 0.3f, y + size * 1.0,
            "ty", 0.7f, y + size * 0.0,
            "ty", 1.0f, y + size - size * ((double)amount/100),
            NULL);
            */

    one = NUM_CREATE(group, COLOR_TEXT, 5, textsize, 0);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, -size, y + size + 20);

    one0 = NUM_CREATE(group, COLOR_TEXT, 5, textsize, 0);
    nemoshow_item_set_alpha(one0, 0.0);
    nemoshow_item_translate(one0, -size, y + size + 20);

    one1 = _path_create(show, group, textsize - 10, PATH_PERCENT, NULL);
    _path_set_fill(one1, COLOR_TEXT, 0.0);
    nemoshow_item_translate(one1, -size, y + size + 20);
    weather->now.rsc0 = one;
    weather->now.rsc1 = one0;
    weather->now.rsc2 = one1;

    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 1.0f,
            "tx",    x + 25.0f,
            NULL);

    _nemoshow_item_motion(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 1.0f,
            "tx",    x + 60.0f,
            NULL);

    _nemoshow_item_motion(one1, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x + 100.0f,
            NULL);

    _nemoshow_item_motion(ONE_CHILD0(one1), NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "alpha", 1.0f,
            NULL);

    if (amount >= 99) {
        NUM_UPDATE(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 1000, 9);
        NUM_UPDATE(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 1000, 9);
    } else {
        NUM_UPDATE(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 1000, amount/10);
        NUM_UPDATE(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay + 1000, amount%10);
    }

    // Pressure
    x += 250;
    one = PATH_CREATE(group);
    nemoshow_item_set_fill_color(one, RGBA(COLOR_BASE));
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, -size, y);
    nemoshow_item_path_moveto(one, -5, -5);
    nemoshow_item_path_lineto(one, size + 5, 0);
    nemoshow_item_path_lineto(one, size + 5, size + 5);
    nemoshow_item_path_lineto(one, 0, size + 5);
    nemoshow_item_path_close(one);
    weather->now.pressure_clip = one;

    one = _path_create(show, group, size, PATH_THERMOMETER, NULL);
    _path_set_fill(one, COLOR_BASE, 1.0);
    nemoshow_item_translate(one, -size, y);
    nemoshow_item_set_clip(one, weather->now.pressure_clip);
    weather->now.pressure_in = one;

    one = _path_create(show, group, size, PATH_THERMOMETER, NULL);
    _path_set_stroke(one, COLOR_BASE, 1.0, 5);
    nemoshow_item_translate(one, -size, y);
    weather->now.pressure = one;

    double per = 0.0;
    if (w->pressure > 1000) per = 1.0;
    else if (w->pressure < 800) per = 0;
    else {
        per = (w->pressure - 1000 + 200)/(double)200;
        ERR("%d --> %lf", w->pressure, per);
    }
    _nemoshow_item_motion(weather->now.pressure, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "tx", x,
            NULL);
    _nemoshow_item_motion(weather->now.pressure_in, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "tx", x,
            NULL);
    _nemoshow_item_motion(weather->now.pressure_clip, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "tx", x,
            NULL);

    m = nemomotion_create(show, NEMOEASE_CUBIC_OUT_TYPE, dur * 5, delay + 1000);
    nemomotion_attach(m, 0.3, weather->now.pressure_clip, "ty", y + size * 1.0, NULL);
    nemomotion_attach(m, 0.7, weather->now.pressure_clip, "ty", y + size * 0.0, NULL);
    nemomotion_attach(m, 1.0, weather->now.pressure_clip, "ty", y + size - size * per, NULL);
    nemomotion_run(m);
    /*
    _nemoshow_item_motion(weather->now.pressure_clip, NEMOEASE_CUBIC_OUT_TYPE, dur * 5, delay + 1000,
            "ty", 0.3f, y + size * 1.0,
            "ty", 0.7f, y + size * 0.0,
            "ty", 1.0f, y + size - size * per,
            NULL);
            */

    amount = w->pressure;
    one = NUM_CREATE(group, COLOR_TEXT, 5, textsize, 0);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, -size, y + size + 20);

    one0 = NUM_CREATE(group, COLOR_TEXT, 5, textsize, 0);
    nemoshow_item_set_alpha(one0, 0.0);
    nemoshow_item_translate(one0, -size, y + size + 20);

    one1 = NUM_CREATE(group, COLOR_TEXT, 5, textsize, 0);
    nemoshow_item_set_alpha(one1, 0.0);
    nemoshow_item_translate(one1, -size, y + size + 20);

    one2 = NUM_CREATE(group, COLOR_TEXT, 5, textsize, 0);
    nemoshow_item_set_alpha(one2, 0.0);
    nemoshow_item_translate(one2, -size, y + size + 20);

    weather->now.pressure0 = one;
    weather->now.pressure1 = one0;
    weather->now.pressure2 = one1;
    weather->now.pressure3 = one2;

    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "alpha", 1.0f,
            "tx",    x - 15,
            NULL);
    _nemoshow_item_motion(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "alpha", 1.0f,
            "tx",    x + 20.0f,
            NULL);
    _nemoshow_item_motion(one1, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "alpha", 1.0f,
            "tx",    x + 55.0f,
            NULL);
    _nemoshow_item_motion(one2, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "alpha", 1.0f,
            "tx",    x + 90.0f,
            NULL);

    if (amount >= 9999) {
        NUM_UPDATE(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, 1000, 9);
        NUM_UPDATE(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, 1000, 9);
        NUM_UPDATE(one1, NEMOEASE_CUBIC_INOUT_TYPE, dur, 1000, 9);
        NUM_UPDATE(one2, NEMOEASE_CUBIC_INOUT_TYPE, dur, 1000, 9);
    } else {
        NUM_UPDATE(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, 1000, amount/1000);
        NUM_UPDATE(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, 1000, (amount/100)%10);
        NUM_UPDATE(one1, NEMOEASE_CUBIC_INOUT_TYPE, dur, 1000, (amount/10)%10);
        NUM_UPDATE(one2, NEMOEASE_CUBIC_INOUT_TYPE, dur, 1000, amount%10);
    }

    // Wind
    x += 250;
    one = _path_create(show, group, size, PATH_PINWHEEL, NULL);
    _path_set_fill(one, COLOR_BASE, 0.0);
    nemoshow_item_translate(one, 1024 + size, y);

    _nemoshow_item_motion(ONE_CHILD0(one), NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "alpha", 1.0f,
            NULL);
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "tx", x,
            NULL);
    _nemoshow_item_motion(ONE_CHILD0(one), NEMOEASE_CUBIC_INOUT_TYPE, dur * 2, 1000,
            "ro", 360.0 * w->wind_speed,
            NULL);
    weather->now.wind = one;

    amount = w->wind_speed;
    one = _path_create(show, group, textsize, PATH_UP, NULL);
    _path_set_fill(one, COLOR_TEXT, 0.0);
    nemoshow_item_pivot(one, 20, 20);
    nemoshow_item_translate(one, 1024 + size, y + size + 20);

    one0 = NUM_CREATE(group, COLOR_TEXT, 5, textsize, 0);
    nemoshow_item_set_alpha(one0, 0.0);
    nemoshow_item_translate(one0, 1024 + size, y + size + 20);

    one1 = NUM_CREATE(group, COLOR_TEXT, 5, textsize, 0);
    nemoshow_item_set_alpha(one1, 0.0);
    nemoshow_item_translate(one1, 1024 + size, y + size + 20);

    one2 = TEXT_CREATE(group, weather->font, 30, "m/s");
    nemoshow_item_set_fill_color(one2, RGBA(COLOR_TEXT));
    nemoshow_item_set_alpha(one2, 0.0);
    nemoshow_item_translate(one2, 1024 + size, y + size + 25);

    weather->now.wind0 = one;
    weather->now.wind1 = one0;
    weather->now.wind2 = one1;
    weather->now.wind3 = one2;

    _nemoshow_item_motion(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "alpha", 1.0f,
            "tx",    x + 30,
            NULL);
    _nemoshow_item_motion(ONE_CHILD0(one), NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "alpha", 1.0f,
            NULL);
    _nemoshow_item_motion(one1, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "alpha", 1.0f,
            "tx",    x + 65.0f,
            NULL);
    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "tx", x,
            NULL);
    _nemoshow_item_motion(one2, NEMOEASE_CUBIC_INOUT_TYPE, dur, 0,
            "alpha", 1.0f,
            "tx",    x + 110.0f,
            NULL);

#if 0
    if (amount >= 99) {
        NUM_UPDATE(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, 1000, 9);
        NUM_UPDATE(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, 1000, 9);
    } else {
        NUM_UPDATE(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, 1000, amount/10);
        NUM_UPDATE(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, 1000, amount%10);
    }
#endif

    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, 1000,
            "ro", (double)w->wind_degree,
            NULL);

    // Humidity
    delay = 500;
    x += 250;
    one = PATH_CREATE(group);
    nemoshow_item_set_fill_color(one, RGBA(COLOR_BASE));
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, 1024 + size, y);
    nemoshow_item_path_moveto(one, -5, -5);
    nemoshow_item_path_lineto(one, size + 5, 0);
    nemoshow_item_path_lineto(one, size + 5, size + 5);
    nemoshow_item_path_lineto(one, 0, size + 5);
    nemoshow_item_path_close(one);
    weather->now.humidity_clip = one;

    one = _path_create(show, group, size, PATH_RAIN, NULL);
    _path_set_fill(one, COLOR_BASE, 1.0);
    nemoshow_item_translate(one, 1024 + size, y);
    nemoshow_item_set_clip(one, weather->now.humidity_clip);
    weather->now.humidity_in = one;

    one = _path_create(show, group, size, PATH_RAIN, NULL);
    _path_set_stroke(one, COLOR_BASE, 1.0, 3);
    nemoshow_item_translate(one, 1024 + size, y);
    weather->now.humidity = one;

    amount = w->humidity;
    _nemoshow_item_motion(weather->now.humidity, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x,
            NULL);
    _nemoshow_item_motion(weather->now.humidity_in, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x,
            NULL);
    _nemoshow_item_motion(weather->now.humidity_clip, NEMOEASE_CUBIC_INOUT_TYPE, dur, delay,
            "tx", x,
            NULL);
    m = nemomotion_create(show, NEMOEASE_CUBIC_OUT_TYPE, dur * 5, delay + 1000);
    nemomotion_attach(m, 0.3, weather->now.humidity_clip, "ty", y + size * 1.0, NULL);
    nemomotion_attach(m, 0.7, weather->now.humidity_clip, "ty", y + size * 0.0, NULL);
    nemomotion_attach(m, 1.0, weather->now.humidity_clip, "ty", y + size - size * (amount/100.0), NULL);
    nemomotion_run(m);

    /*
    _nemoshow_item_motion(weather->now.humidity_clip, NEMOEASE_CUBIC_OUT_TYPE, dur * 5, delay + 1000,
            "ty", 0.3f, y + size * 1.0,
            "ty", 0.7f, y + size * 0.0,
            "ty", 1.0f, y + size - size * ((double)amount/100),
            NULL);
            */

    one = NUM_CREATE(group, COLOR_TEXT, 5, textsize, 0);
    nemoshow_item_set_alpha(one, 0.0);
    nemoshow_item_translate(one, 1024 + size, y + size + 20);
    weather->now.humidity0 = one;

    one0 = NUM_CREATE(group, COLOR_TEXT, 5, textsize, 0);
    nemoshow_item_set_alpha(one0, 0.0);
    nemoshow_item_translate(one0, 1024 + size, y + size + 20);
    weather->now.humidity1 = one0;

    one1 = _path_create(show, group, textsize - 10, PATH_PERCENT, NULL);
    _path_set_fill(one1, COLOR_TEXT, 0.0);
    nemoshow_item_translate(one1, 1024 + size, y + size + 20);
    weather->now.humidity2 = one1;

    _nemoshow_item_motion(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, 500,
            "alpha", 1.0f,
            "tx",    x + 25.0f,
            NULL);
    _nemoshow_item_motion(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, 500,
            "alpha", 1.0f,
            "tx",    x + 60.0f,
            NULL);
    _nemoshow_item_motion(one1, NEMOEASE_CUBIC_INOUT_TYPE, dur, 500,
            "tx", x + 100.0f,
            NULL);
    _nemoshow_item_motion(ONE_CHILD0(one1), NEMOEASE_CUBIC_INOUT_TYPE, dur, 500,
            "alpha", 1.0f,
            NULL);

    if (amount >= 99) {
        NUM_UPDATE(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, 500 + 1000, 9);
        NUM_UPDATE(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, 500 + 1000, 9);
    } else {
        NUM_UPDATE(one, NEMOEASE_CUBIC_INOUT_TYPE, dur, 500 + 1000, amount/10);
        NUM_UPDATE(one0, NEMOEASE_CUBIC_INOUT_TYPE, dur, 500 + 1000, amount%10);
    }
    nemoshow_dispatch_frame(weather->show);
}

static void _weather_now_reset_timer(Weather *weather)
{
    nemotimer_set_timeout(weather->now.update_timer,
            weather->now.update_window * 1000);

    weather->now.update_window = weather->now.update_window * 2;
    if (weather->now.update_window >= UPDATE_TIME)
        weather->now.update_window = UPDATE_TIME;
}

static void _weather_now_get_complete_cb(Con *con, bool success, char *data, size_t size, void *userdata)
{
    Weather *weather = userdata;
    if (!success || !data || size <= 0) {
        ERR("Connect failed, Request again after %ds",
                weather->now.update_window);
        _weather_now_reset_timer(weather);
        return;
    }

    char *json = malloc(size + 1);
    memcpy(json, data, size);
    json[size] = '\0';

    //fprintf(stderr, "%s", json);
    WeatherNow *w = _weather_now_create_openweather(json);
    free(json);
    if (w) {
        if (weather->now.w) _weather_now_destroy(weather->now.w);
        weather->now.w = w;

        //_weather_now_dump(w);
        _weather_now_hide(weather);
        _weather_now_show(weather);

        nemoshow_dispatch_frame(weather->show);
        weather->now.update_window = UPDATE_WINDOW;
        nemotimer_set_timeout(weather->now.update_timer, UPDATE_TIME);
    } else {
        ERR("Parsing failed, Request again after %ds",
                weather->now.update_window);
        _weather_now_reset_timer(weather);
    }
}

static void _weather_daily_reset_timer(Weather *weather)
{
    nemotimer_set_timeout(weather->daily.update_timer,
            weather->daily.update_window * 1000);

    weather->daily.update_window = weather->daily.update_window * 2;
    if (weather->daily.update_window >= UPDATE_TIME)
        weather->daily.update_window = UPDATE_TIME;
}

static void _weather_daily_get_complete_cb(Con *con, bool success, char *data, size_t size, void *userdata)
{
    Weather *weather = userdata;
    if (!success || !data || size <= 0) {
        ERR("Connect failed, Request again after %ds",
                weather->daily.update_window);
        _weather_daily_reset_timer(weather);
        return;
    }

    char *json = malloc(size + 1);
    memcpy(json, data, size);
    json[size] = '\0';

    //fprintf(stderr, "%s", json);
    WeatherDaily *w = _weather_daily_create_openweather(json);
    free(json);
    if (w) {
        if (weather->daily.w) _weather_daily_destroy(weather->daily.w);
        weather->daily.w = w;

        //_weather_daily_dump(w);
        _weather_daily_hide(weather);
        _weather_daily_show(weather);
        nemoshow_dispatch_frame(weather->show);

        weather->daily.update_window = UPDATE_WINDOW;
        nemotimer_set_timeout(weather->daily.update_timer, UPDATE_TIME);
    } else {
        ERR("Parsing failed, Request again after %ds",
                weather->daily.update_window);
        _weather_daily_reset_timer(weather);
    }
}

static void _weather_now_get(Weather *weather)
{
    char *url;
    url = _openweather_url_get(OPENWEATHER_TYPE_NOW,
            weather->city, weather->country, -1, 0, 0);

    if (weather->now.con) con_destroy(weather->now.con);
    Con *con;
    con = con_create(weather->tool);
    con_set_url(con, url);
    con_set_end_callback(con, _weather_now_get_complete_cb, weather);
    con_run(con);
    weather->now.con = con;
    free(url);
}
static void _weather_daily_get(Weather *weather)
{
    char *url;
    url= _openweather_url_get(OPENWEATHER_TYPE_DAILY,
            weather->city, weather->country, -1, 0, 0);


    if (weather->daily.con) con_destroy(weather->daily.con);
    Con *con;
    con = con_create(weather->tool);
    con_set_url(con, url);
    con_set_end_callback(con, _weather_daily_get_complete_cb, weather);
    con_run(con);
    weather->daily.con = con;
    free(url);
}

static void _weather_now_update(struct nemotimer *timer, void *userdata)
{
    Weather *weather = userdata;
    _weather_now_get(weather);
    nemotimer_set_timeout(timer, UPDATE_TIME);
}

static void _weather_daily_update(struct nemotimer *timer, void *userdata)
{
    Weather *weather = userdata;
    _weather_daily_get(weather);
    nemotimer_set_timeout(timer, UPDATE_TIME);
}

static void _weather_destroy(Weather *weather)
{
    nemoshow_one_destroy(weather->font);
    nemoshow_one_destroy(weather->group);
}

static Weather *_weather_create(struct nemotool *tool, struct showone *parent, int width, int height, const char *city, const char *country)
{
    Weather *weather = calloc(sizeof(Weather), 1);
    weather->tool = tool;
    weather->show = parent->show;
    weather->parent = parent;
    weather->city = city;
    weather->country = country;

    weather->font = FONT_CREATE("BM JUA", "Regular");
    weather->group = GROUP_CREATE(parent);

    struct showone *one;
    // border
    int sw = 5;
    int len = 50;
    one = PATH_CREATE(weather->group);
    nemoshow_item_set_stroke_color(one, RGBA(COLOR_BASE));
    nemoshow_item_set_stroke_width(one, sw);
    nemoshow_item_set_alpha(one, 0.0);

    int x = 12;
    int y = 12;
    nemoshow_item_path_moveto(one, x, len);
    nemoshow_item_path_cubicto(one, x, y, x, y, len, y);
    nemoshow_item_path_moveto(one, width - len, y);
    nemoshow_item_path_cubicto(one, width - x, y, width - x, y, width - x, len);
    nemoshow_item_path_moveto(one, width - x, height - len);
    nemoshow_item_path_cubicto(one, width - x, height - y, width - x, height - y,
            width - len, height - y);
    nemoshow_item_path_moveto(one, len, height - y);
    nemoshow_item_path_cubicto(one, x, height - y, x, height - y, x, height - len);
    weather->bg = one;

    return weather;
}

static void _weather_show(Weather *weather, uint32_t easetype, int duration, int delay)
{
    struct nemotool *tool = weather->tool;
    _nemoshow_item_motion(weather->bg, easetype, duration, delay,
            "alpha", 1.0,
            NULL);

    struct nemotimer *timer;
    timer = nemotimer_create(tool);
    nemotimer_set_timeout(timer, 100);
    nemotimer_set_callback(timer, _weather_now_update);
    nemotimer_set_userdata(timer, weather);
    weather->now.update_window = UPDATE_WINDOW;
    weather->now.update_timer = timer;

    timer = nemotimer_create(tool);
    nemotimer_set_timeout(timer, 100);
    nemotimer_set_callback(timer, _weather_daily_update);
    nemotimer_set_userdata(timer, weather);
    weather->daily.update_window = UPDATE_WINDOW;
    weather->daily.update_timer = timer;

    nemoshow_dispatch_frame(weather->show);
}

static void _weather_hide(Weather *weather, uint32_t easetype, int duration, int delay)
{
    _nemoshow_item_motion(weather->bg, easetype, duration, delay,
            "alpha", 0.0,
            NULL);

    _weather_now_hide(weather);
    _weather_daily_hide(weather);

    nemoshow_dispatch_frame(weather->show);
}

static void _win_exit(NemoWidget *win, const char *id, void *info, void *userdata)
{
    Weather *weather = userdata;
    _nemoshow_destroy_transition_all(weather->show);

    _weather_hide(weather, NEMOEASE_CUBIC_IN_TYPE, 1500, 0);
    nemowidget_win_exit_if_no_trans(win);
}

typedef struct _ConfigApp ConfigApp;
struct _ConfigApp {
    Config *config;
    char *city;
    char *country;
};

static ConfigApp *_config_load(const char *domain, const char *filename, int argc, char *argv[])
{
    ConfigApp *app = calloc(sizeof(ConfigApp), 1);
    app->config = config_load(domain, filename, argc, argv);

    Xml *xml;
    if (app->config->path) {
        xml = xml_load_from_path(app->config->path);
        if (!xml) ERR("Load configuration failed: %s", app->config->path);
    } else {
        xml = xml_load_from_domain(domain, filename);
        if (!xml) ERR("Load configuration failed: %s:%s", domain, filename);
    }
    if (xml) {
        const char *root = "config";
        char buf[PATH_MAX];
        const char *temp;
        snprintf(buf, PATH_MAX, "%s/geometry", root);

        temp = xml_get_value(xml, buf, "city");
        if (temp && strlen(temp) > 0) {
            app->city = strdup(temp);
        }
        temp = xml_get_value(xml, buf, "country");
        if (temp && strlen(temp) > 0) {
            app->country = strdup(temp);
        }
        xml_unload(xml);
    }

    if (!app->city) {
        ERR("No city in the configuration (Default is Seoul)");
        app->city = strdup("Seoul");
    }
    if (!app->country) {
        ERR("No country in the configuration (Default is kr)");
        app->country = strdup("kr");
    }
    return app;
}

static void _config_unload(ConfigApp *app)
{
    config_unload(app->config);
    free(app->city);
    free(app->country);
    free(app);
}

int main(int argc, char *argv[])
{
    int sw = 1024, sh = 1024;
    ConfigApp *config = _config_load(PROJECT_NAME, CONFXML, argc, argv);
    RET_IF(!config, -1);

    con_init();

    struct nemotool *tool = TOOL_CREATE();
    NemoWidget *win = nemowidget_create_win(tool, APPNAME, config->config->width, config->config->height);
    nemowidget_win_load_scene(win, sw, sh);

    NemoWidget *vector;
    vector = nemowidget_create_vector(win, sw, sh);
    nemowidget_show(vector, 0, 0, 0);

    // FIXME: Desgined for 1024x1024
    Weather *weather = _weather_create(nemowidget_get_tool(vector),
            nemowidget_get_canvas(vector), sw, sh, config->city, config->country);
    nemowidget_append_callback(win, "exit", _win_exit, weather);

    _weather_show(weather, NEMOEASE_CUBIC_OUT_TYPE, 1000, 0);

    nemowidget_show(win, 0, 0, 0);
    nemotool_run(tool);

    _weather_destroy(weather);
    nemowidget_destroy(vector);
    nemowidget_destroy(win);
    TOOL_DESTROY(tool);
    _config_unload(config);

    con_shutdown();

    return 0;
}
