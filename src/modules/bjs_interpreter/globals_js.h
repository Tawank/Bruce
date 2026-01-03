#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#ifndef __GLOBALS_JS_H__
#define __GLOBALS_JS_H__

#include "helpers_js.h"

extern "C" {
JSValue js_gc(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_load(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_setTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_clearTimeout(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

JSValue js_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_date_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_performance_now(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue js_require(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

#endif
#endif
