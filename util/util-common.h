#ifndef __UTIL_COMMON_H__
#define __UTIL_COMMON_H__

#include <stdint.h>
#include <stdbool.h>

#define EPSILON 0.001
#define EQUAL(a, b) (((a) > (b)) ? (((a) - (b)) < EPSILON) : (((b) - (a)) < EPSILON))
#define DISTANCE(x, y, xx, yy) sqrt(pow((x) - (xx), 2.0) + pow((y) - (yy), 2.0))

#define CIRCLE_IN(x, y, r, xx, yy) \
     (DISTANCE((x), (y), (xx), (yy)) <= (r)) ? true : false

#define CIRCLES_CROSS(x, y, r, xx, yy, rr) \
     (DISTANCE((x), (y), (xx), (yy)) <= (r + rr)) ? true : false

#define RECTS_CROSS(x, y, w, h, xx, yy, ww, hh) \
    (((((int)x) + ((int)w)) >= ((int)xx)) && ((((int)y) + ((int)h)) >= ((int)yy)) && \
    ((((int)xx) + ((int)ww)) >= ((int)x)) && ((((int)yy) + ((int)hh)) >= ((int)y)))

#ifdef __cplusplus
extern "C" {
#endif

void _rect_ratio_full(int w, int h, int cw, int ch, int *_w, int *_h);
void _rect_ratio_fit(int w, int h, int cw, int ch, int *_w, int *_h);
char* strdup_printf(const char *fmt, ...);
char *str_replace(char *str, const char *org, const char *rep);

typedef enum {
    VALUE_TYPE_NONE = 0,
    VALUE_TYPE_INT,
    VALUE_TYPE_DOUBLE,
    VALUE_TYPE_STRING,
    VALUE_TYPE_POINTER,
    VALUE_TYPE_MAX,
} VALUE_TYPE;

#define VALUE_INIT {VALUE_TYPE_NONE, 0, 0.0, NULL, NULL}
typedef struct _Value Value;
struct _Value {
    VALUE_TYPE type;
    int v_int;
    double v_double;
    char *v_str;
    char *v_ptr;
};

Value *value_create();
void value_set_int(Value *val, int _int);
void value_set_double(Value *val, double _double);
void value_set_str(Value *val, const char *_str);
void value_set_ptr(Value *val, void *_ptr);
Value *value_dup(Value *val);
void value_free(Value *val);

#define VALUES_INIT {0, 0}
typedef struct _Values Values;
struct _Values {
    int cnt;
    Value *vals;
};

Values *values_create();
void values_set_val(Values *vals, int idx, Value *val);
void values_set_int(Values *vals, int idx, int _int);
void values_set_double(Values *vals, int idx, double _double);
void values_set_str(Values *vals, int idx, const char *_str);
void values_set_ptr(Values *vals, int idx, void *_ptr);
Value *values_dup(Values *vals, int idx);
Value *values_get_val(Values *vals, int idx);
int values_get_int(Values *vals, int idx);
double values_get_double(Values *vals, int idx);
const char *values_get_str(Values *vals, int idx);
void *values_get_ptr(Values *vals, int idx);
void values_free(Values *vals);

int math_accumulate(int f);
uint32_t get_time(void);

typedef struct _Clock Clock;
struct _Clock {
    int year;
    int month;
    int day;
    int week;
    int hours;
    int mins;
    int secs;
};

Clock clock_get();

void WELLRNG512_INIT();
unsigned long WELLRNG512(void);


static inline void parse_seconds_to_hms(int64_t seconds, int *hour, int *min, int *sec)
{
    if (hour) *hour = seconds/3600;
    if (min) *min = (seconds%3600)/60;
    if (sec) *sec = (seconds%3600)%60;
}

#ifdef __cplusplus
}
#endif

#endif
