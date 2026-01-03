#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "globals_js.h"

extern "C" {
JSValue js_gc(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    JS_GC(ctx);
    return JS_UNDEFINED;
}

JSValue js_load(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    const char *filename;
    JSCStringBuf buf_str;
    uint8_t *buf;
    int buf_len;
    JSValue ret;

    filename = JS_ToCString(ctx, argv[0], &buf_str);
    if (!filename) return JS_EXCEPTION;

    // TODO: Load file
    // buf = load_file(filename, &buf_len);

    ret = JS_Eval(ctx, (const char *)buf, buf_len, filename, 0);
    free(buf);
    return ret;
}

/* timers */
typedef struct {
    bool allocated;
    JSGCRef func;
    int64_t timeout; /* in ms */
} JSTimer;

#define MAX_TIMERS 16

static JSTimer js_timer_list[MAX_TIMERS];

JSValue js_setTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    JSTimer *th;
    int delay, i;
    JSValue *pfunc;

    if (!JS_IsFunction(ctx, argv[0])) return JS_ThrowTypeError(ctx, "not a function");
    if (JS_ToInt32(ctx, &delay, argv[1])) return JS_EXCEPTION;
    for (i = 0; i < MAX_TIMERS; i++) {
        th = &js_timer_list[i];
        if (!th->allocated) {
            pfunc = JS_AddGCRef(ctx, &th->func);
            *pfunc = argv[0];
            th->timeout = millis() + delay;
            th->allocated = true;
            return JS_NewInt32(ctx, i);
        }
    }
    return JS_ThrowInternalError(ctx, "too many timers");
}

JSValue js_clearTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int timer_id;
    JSTimer *th;

    if (JS_ToInt32(ctx, &timer_id, argv[0])) return JS_EXCEPTION;
    if (timer_id >= 0 && timer_id < MAX_TIMERS) {
        th = &js_timer_list[timer_id];
        if (th->allocated) {
            JS_DeleteGCRef(ctx, &th->func);
            th->allocated = false;
        }
    }
    return JS_UNDEFINED;
}

static void run_timers(JSContext *ctx) {
    int64_t min_delay, delay, cur_time;
    bool has_timer;
    int i;
    JSTimer *th;
    struct timespec ts;

    for (;;) {
        min_delay = 1000;
        cur_time = millis();
        has_timer = false;
        for (i = 0; i < MAX_TIMERS; i++) {
            th = &js_timer_list[i];
            if (th->allocated) {
                has_timer = true;
                delay = th->timeout - cur_time;
                if (delay <= 0) {
                    JSValue ret;
                    /* the timer expired */
                    if (JS_StackCheck(ctx, 2)) goto fail;
                    JS_PushArg(ctx, th->func.val); /* func name */
                    JS_PushArg(ctx, JS_NULL);      /* this */

                    JS_DeleteGCRef(ctx, &th->func);
                    th->allocated = false;

                    ret = JS_Call(ctx, 0);
                    if (JS_IsException(ret)) {
                    fail:
                        log_d("Error in run_timers");
                        // TODO: proper error handling
                        // dump_error(ctx);
                        // exit(1);
                    }
                    min_delay = 0;
                    break;
                } else if (delay < min_delay) {
                    min_delay = delay;
                }
            }
        }
        if (!has_timer) break;
        if (min_delay > 0) {
            ts.tv_sec = min_delay / 1000;
            ts.tv_nsec = (min_delay % 1000) * 1000000;
            nanosleep(&ts, NULL);
        }
    }
}

JSValue js_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int i;
    JSValue v;

    for (i = 0; i < argc; i++) {
        if (i != 0) putchar(' ');
        v = argv[i];
        if (JS_IsString(ctx, v)) {
            JSCStringBuf buf;
            const char *str;
            size_t len;
            str = JS_ToCStringLen(ctx, &len, v, &buf);
            fwrite(str, 1, (size_t)len, stdout);
        } else {
            JS_PrintValueF(ctx, argv[i], JS_DUMP_LONG);
        }
    }
    putchar('\n');
    return JS_UNDEFINED;
}

JSValue js_date_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return JS_NewInt64(ctx, (int64_t)tv.tv_sec * 1000 + (tv.tv_usec / 1000));
}

JSValue js_performance_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return JS_NewInt64(ctx, (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL)));
}

JSValue js_require(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1) { return JS_ThrowTypeError(ctx, "require() expects 1 argument"); }

    JSCStringBuf name_buf;
    const char *name = JS_ToCString(ctx, argv[0], &name_buf);
    if (!name) { return JS_EXCEPTION; }

    JSValue global = JS_GetGlobalObject(ctx);
    JSValue val = JS_GetPropertyStr(ctx, global, name);

    return val;
}
}

#endif
