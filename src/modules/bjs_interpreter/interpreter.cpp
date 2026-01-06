#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "interpreter.h"
#include "core/utils.h"

static void js_log_func(void *opaque, const void *buf, size_t buf_len) { fwrite(buf, 1, buf_len, stdout); }

extern "C" {
#include "mqjs_stdlib.h"
}

#include "display_js.h"

char *script = NULL;
char *scriptDirpath = NULL;
char *scriptName = NULL;

void interpreterHandler(void *pvParameters) {
    printMemoryUsage("init interpreter");
    if (script == NULL) { return; }
    tft.fillScreen(TFT_BLACK);
    tft.setRotation(bruceConfigPins.rotation);
    tft.setTextSize(FM);
    tft.setTextColor(TFT_WHITE);

    bool psramAvailable = psramFound();

    size_t mem_size = psramAvailable ? 65536 : 32768;
    uint8_t *mem_buf = psramAvailable ? (uint8_t *)ps_malloc(mem_size) : (uint8_t *)malloc(mem_size);
    JSContext *ctx = JS_NewContext(mem_buf, mem_size, &js_stdlib);
    JS_SetLogFunc(ctx, js_log_func);

    // Set global variables
    JSValue global = JS_GetGlobalObject(ctx);
    JS_SetPropertyStr(
        ctx, global, "__filepath", JS_NewString(ctx, (String(scriptDirpath) + String(scriptName)).c_str())
    );
    JS_SetPropertyStr(ctx, global, "__dirpath", JS_NewString(ctx, scriptDirpath));
    JS_SetPropertyStr(ctx, global, "BRUCE_VERSION", JS_NewString(ctx, BRUCE_VERSION));
    JS_SetPropertyStr(ctx, global, "BRUCE_PRICOLOR", JS_NewInt32(ctx, bruceConfig.priColor));
    JS_SetPropertyStr(ctx, global, "BRUCE_SECCOLOR", JS_NewInt32(ctx, bruceConfig.secColor));
    JS_SetPropertyStr(ctx, global, "BRUCE_BGCOLOR", JS_NewInt32(ctx, bruceConfig.bgColor));

    JS_SetPropertyStr(ctx, global, "HIGH", JS_NewInt32(ctx, HIGH));
    JS_SetPropertyStr(ctx, global, "LOW", JS_NewInt32(ctx, LOW));
    JS_SetPropertyStr(ctx, global, "INPUT", JS_NewInt32(ctx, INPUT));
    JS_SetPropertyStr(ctx, global, "OUTPUT", JS_NewInt32(ctx, OUTPUT));
    JS_SetPropertyStr(ctx, global, "PULLUP", JS_NewInt32(ctx, PULLUP));
    JS_SetPropertyStr(ctx, global, "INPUT_PULLUP", JS_NewInt32(ctx, INPUT_PULLUP));
    JS_SetPropertyStr(ctx, global, "PULLDOWN", JS_NewInt32(ctx, PULLDOWN));
    JS_SetPropertyStr(ctx, global, "INPUT_PULLDOWN", JS_NewInt32(ctx, INPUT_PULLDOWN));

    // Init containers
    clearDisplayModuleData();

    printMemoryUsage("context created");

    size_t scriptSize = strlen(script);
    log_d("Script length: %zu\n", scriptSize);

    JSValue val = JS_Eval(ctx, (const char *)script, scriptSize, scriptName, 0);

    run_timers(ctx);

    free((char *)script);
    script = NULL;
    free((char *)scriptDirpath);
    scriptDirpath = NULL;
    free((char *)scriptName);
    scriptName = NULL;

    if (JS_IsException(val)) { js_fatal_error_handler(ctx); }

    JS_FreeContext(ctx);
    free(mem_buf);

    // Clean up.
    clearDisplayModuleData();
    // TODO: if backgroud app implemented, store in ctx and set if on foreground/background
    LongPress = false;

    interpreter_start = false;
    vTaskDelete(NULL);
    return;
}

// function to start the JS Interpreterm choosinng the file, processing and
// start
void run_bjs_script() {
    String filename;
    FS *fs = &LittleFS;
    setupSdCard();
    if (sdcardMounted) {
        options = {
            {"SD Card",  [&]() { fs = &SD; }      },
            {"LittleFS", [&]() { fs = &LittleFS; }},
        };
        loopOptions(options);
    }
    filename = loopSD(*fs, true, "BJS|JS");
    script = readBigFile(fs, filename);
    if (script == NULL) { return; }

    returnToMenu = true;
    interpreter_start = true;

    // To stop the script, press Prev and Next together for a few seconds
}

bool run_bjs_script_headless(char *code) {
    script = code;
    if (script == NULL) { return false; }
    scriptDirpath = strdup("/scripts");
    scriptName = strdup("index.js");
    returnToMenu = true;
    interpreter_start = true;
    return true;
}

bool run_bjs_script_headless(FS fs, String filename) {
    script = readBigFile(&fs, filename);
    if (script == NULL) { return false; }

    int slash = filename.lastIndexOf('/');
    scriptName = strdup(filename.c_str() + slash + 1);
    scriptDirpath = strndup(filename.c_str(), slash);

    returnToMenu = true;
    interpreter_start = true;
    return true;
}

const char *nth_strchr(const char *s, char c, int8_t n) {
    const char *nth = s;
    if (c == '\0' || n < 1) return NULL;

    for (int i = 0; i < n; i++) {
        if ((nth = strchr(nth, c)) == 0) break;
        nth++;
    }

    return nth;
}

/* 2FIX: not working
// terminate the script
duk_ret_t native_exit(duk_context *ctx) {
  duk_error(ctx, DUK_ERR_ERROR, "Script exited");
  interpreter_start=false;
  return 0;
}
*/

#endif
