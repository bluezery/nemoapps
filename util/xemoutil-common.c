#include "xemoutil-internal.h"
#include "xemoutil.h"
#include "xemoutil-common.h"

void _rect_ratio_full(int w, int h, int cw, int ch, int *_w, int *_h)
{
    double diff_w = (double)cw/w;
    double diff_h = (double)ch/h;

    if (diff_w <= diff_h) {
        w = w * diff_h;
        h = h * diff_h;
    } else if (diff_w > diff_h) {
        w = w * diff_w;
        h = h * diff_w;
    }


    if (_w) *_w = w;
    if (_h) *_h = h;

}

void _rect_ratio_fit(int w, int h, int cw, int ch, int *_w, int *_h)
{
    double diff_w = (double)cw/w;
    double diff_h = (double)ch/h;

    if (diff_w <= diff_h) {
        w = w * diff_w;
        h = h * diff_w;
    } else if (diff_w > diff_h) {
        w = w * diff_h;
        h = h * diff_h;
    }

    if (_w) *_w = w;
    if (_h) *_h = h;
}

/*****************************************************/
/* String */
/*****************************************************/
char* strdup_printf(const char *fmt, ...)
{
    va_list ap;
    char *str = NULL;
    unsigned int size = 0;

    while (1) {
        int n;
        va_start(ap, fmt);
        n = vsnprintf(str, size, fmt, ap);
        va_end(ap);

        if (n < 0) {
            ERR("vsnprintf failed");
            if (str) free(str);
            break;
        }
        if ((size_t)n < size) break;

        size = n + 1;
        str = (char *)realloc(str, size);
    }
    return str;
}

char *str_replace(char *str, const char *org, const char *rep)
{
    RET_IF(!str, NULL);
    RET_IF(!org, NULL);
    RET_IF(!rep, NULL);


    unsigned int i = 0;
    size_t len = strlen(str);
    size_t org_len = strlen(org);
    size_t rep_len = strlen(rep);

    char *pos = strstr(str, org);

    if (org_len > rep_len) {
        for (i = 0 ; i < rep_len ; i++) {
            *pos = *rep;
            pos++;
            rep++;
        }
        size_t diff = org_len - rep_len;
        memmove(pos, pos + diff, strlen(pos + diff) + 1);
    } else {
        size_t diff = rep_len - org_len;
        str = (char *)realloc(str, sizeof(char) * (len + diff + 1));
        memmove(pos + rep_len, pos + org_len, strlen(pos + org_len) + 1);
        for (i = 0 ; i < rep_len ; i++) {
            *pos = *rep;
            pos++;
            rep++;
        }
    }
    return str;
}

/********************************************************/
/* Generic Value */
/********************************************************/
Value *value_create()
{
    Value *val = (Value *)calloc(sizeof(Value), 1);
    return val;
}

void value_set_int(Value *val, int _int)
{
    val->type = VALUE_TYPE_INT;
    val->v_int = _int;
}

void value_set_double(Value *val, double _double)
{
    val->type = VALUE_TYPE_DOUBLE;
    val->v_double = _double;
}

void value_set_str(Value *val, const char *_str)
{
    val->type = VALUE_TYPE_STRING;
    if (val->v_str) free(val->v_str);
    val->v_str = strdup(_str);
}

void value_set_ptr(Value *val, void *_ptr)
{
    val->type = VALUE_TYPE_POINTER;
    val->v_ptr = (char *)_ptr;
}

Value *value_dup(Value *val)
{
    Value *newval = value_create();
    newval->type = val->type;
    if (val->type == VALUE_TYPE_INT) {
        value_set_int(newval, val->v_int);
    } else if (val->type == VALUE_TYPE_DOUBLE) {
        value_set_double(newval, val->v_double);
    } else if (val->type == VALUE_TYPE_STRING) {
        value_set_str(newval, val->v_str);
    } else if (val->type == VALUE_TYPE_POINTER) {
        value_set_ptr(newval, val->v_ptr);
    }
    return newval;
}

void value_free(Value *val)
{
    if (val->v_str) free(val->v_str);
}

Values *values_create()
{
    Values *vals = (Values *)calloc(sizeof(Values), 1);
    return vals;
}

void values_set_val(Values *vals, int idx, Value *val)
{
    RET_IF(!vals || !val)
    if (vals->cnt <= idx) {
        Value *temp = vals->vals;
        vals->vals = (Value *)calloc(sizeof(Value), idx + 1);
        if (temp) {
            memcpy(vals->vals, temp, sizeof(Value) * vals->cnt);
            free(temp);
        }
        vals->cnt = idx + 1;
    }
    vals->vals[idx] = *val;
}

void values_set_int(Values *vals, int idx, int _int)
{
    Value val = VALUE_INIT;
    value_set_int(&val, _int);
    values_set_val(vals, idx, &val);
}

void values_set_double(Values *vals, int idx, double _double)
{
    Value val = VALUE_INIT;
    value_set_double(&val, _double);
    values_set_val(vals, idx, &val);
}

void values_set_str(Values *vals, int idx, const char *_str)
{
    Value val = VALUE_INIT;
    value_set_str(&val, _str);
    values_set_val(vals, idx, &val);
}

void values_set_ptr(Values *vals, int idx, void *_ptr)
{
    Value val = VALUE_INIT;
    value_set_ptr(&val, _ptr);
    values_set_val(vals, idx, &val);
}

Value *values_dup(Values *vals, int idx)
{
    if (vals->cnt <= idx) {
        ERR("idx(%d) exceeds cnt(%d)", idx, vals->cnt);
        return NULL;
    }
    return value_dup(&(vals->vals[idx]));
}

Value *values_get_val(Values *vals, int idx)
{
    RET_IF(!vals, NULL);
    if (vals->cnt <= idx) {
        ERR("idx(%d) exceeds cnt(%d)", idx, vals->cnt);
        return NULL;
    }
    return &(vals->vals[idx]);
}

int values_get_int(Values *vals, int idx)
{
    if (vals->cnt <= idx) {
        ERR("idx(%d) exceeds cnt(%d)", idx, vals->cnt);
        return 0;
    }

    if (vals->vals[idx].type == VALUE_TYPE_DOUBLE)
        return vals->vals[idx].v_double;
    else if (vals->vals[idx].type == VALUE_TYPE_INT)
        return vals->vals[idx].v_int;

    ERR("Nor double or integer value (%d)", vals->vals[idx].type);
    return 0;
}

double values_get_double(Values *vals, int idx)
{
    if (vals->cnt <= idx) {
        ERR("idx(%d) exceeds cnt(%d)", idx, vals->cnt);
        return 0;
    }
    if (vals->vals[idx].type == VALUE_TYPE_DOUBLE)
        return vals->vals[idx].v_double;
    else if (vals->vals[idx].type == VALUE_TYPE_INT)
        return vals->vals[idx].v_int;

    ERR("Nor double or int value (%d)", vals->vals[idx].type);
    return 0;
}

const char *values_get_str(Values *vals, int idx)
{
    if (vals->cnt <= idx) {
        ERR("idx(%d) exceeds cnt(%d)", idx, vals->cnt);
        return 0;
    }
    if (vals->vals[idx].type != VALUE_TYPE_STRING) {
        ERR("Not string value (%d)", vals->vals[idx].type);
        return 0;
    }
    return vals->vals[idx].v_str;
}

void *values_get_ptr(Values *vals, int idx)
{
    if (vals->cnt <= idx) {
        ERR("idx(%d) exceeds cnt(%d)", idx, vals->cnt);
        return 0;
    }
    if (vals->vals[idx].type != VALUE_TYPE_POINTER) {
        ERR("Not pointer value (%d)", vals->vals[idx].type);
        return 0;
    }
    return vals->vals[idx].v_ptr;
}

void values_free(Values *vals)
{
    int i = 0;
    for (i = 0; i < vals->cnt ; i++) {
        value_free(&(vals->vals[i]));
    }
    free(vals->vals);
}

/**********************************************************/
/* math */
/**********************************************************/
int math_accumulate(int f)
{
    int sum = 0;
    while (f > 0) {
        sum += f;
        f--;
    }
    return sum;
}

uint32_t get_time(void)
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/**********************************************************/
/* Miscellaneous */
/**********************************************************/
Clock clock_get()
{
    Clock now;
    time_t nowt;
    struct tm *tm;

    time(&nowt);
    tm = localtime(&nowt);
    now.year = tm->tm_year + 1900;
    now.month = tm->tm_mon + 1;
    now.day = tm->tm_mday;
    now.week = tm->tm_wday;
    now.hours = tm->tm_hour;
    now.mins = tm->tm_min;
    now.secs = tm->tm_sec;
    //printf("%d/%d/%d %d:%d:%d\n", now.year, now.month, now.day, now.hours, now.mins, now.secs);
    return now;
}

static unsigned long wellrng512_state[16] =
{
    1999, 1981, 33, 18, 27,
    9898231312, 25563120, 368349, 345778, 3094,
    5015043, 34252345645, 97764, 2309450, 209569405,
    795030923,
};
static unsigned int _index = 0;

void WELLRNG512_INIT(void)
{
    srand(time(NULL));
    int i;
    for (i = 0 ; i < 16 ; i++) {
        wellrng512_state[i] = rand();
    }
}

unsigned long WELLRNG512(void)
{
    unsigned long a, b, c, d;
    a = wellrng512_state[_index];
    c = wellrng512_state[(_index + 13) & 15];
    b = a^c^(a<<16)^(c<<15);
    c = wellrng512_state[(_index + 9) & 15];
    d = a^((a<<5) & 0xDA442D20UL);
    _index = (_index + 15) & 15;
    a = wellrng512_state[_index];
    wellrng512_state[_index] = a^b^d^(a<<2)^(b<<18)^(c<<28);

    return abs(wellrng512_state[_index]);
}
