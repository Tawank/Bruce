#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "interpreter.h"

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
    script = readBigFile(*fs, filename);
    if (script == NULL) { return; }

    returnToMenu = true;
    interpreter_start = true;

    // To stop the script, press Prev and Next together for a few seconds
}

bool run_bjs_script_headless(char *code) {
    script = code;
    if (script == NULL) { return false; }
    scriptDirpath = strdup("/scripts");
    scriptName = strdup("serial_input.js");
    returnToMenu = true;
    interpreter_start = true;
    return true;
}

bool run_bjs_script_headless(FS fs, String filename) {
    script = readBigFile(fs, filename);
    if (script == NULL) { return false; }

    int slash = filename.lastIndexOf('/');
    scriptName = strndup(filename.c_str(), slash);
    scriptDirpath = strdup(filename.c_str() + slash + 1);

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

void js_fatal_error_handler(void *udata, const char *msg) {
    (void)udata;
    tft.setTextSize(FM);
    tft.setTextColor(TFT_RED, bruceConfig.bgColor);
    tft.drawCentreString("Error", tftWidth / 2, 10, 1);
    tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
    tft.setTextSize(FP);
    tft.setCursor(0, 33);

    tft.printf("JS FATAL ERROR: %s\n", (msg != NULL ? msg : "no message"));
    Serial.printf("JS FATAL ERROR: %s\n", (msg != NULL ? msg : "no message"));
    Serial.flush();

    delay(500);
    while (!check(AnyKeyPress)) vTaskDelay(50 / portTICK_PERIOD_MS);
    // We need to restart esp32 after fatal error
    abort();
}

/* 2FIX: not working
// terminate the script
duk_ret_t native_exit(duk_context *ctx) {
  duk_error(ctx, DUK_ERR_ERROR, "Script exited");
  interpreter_start=false;
  return 0;
}
*/

/*
duk_ret_t native_require(duk_context *ctx) {
    duk_idx_t obj_idx = duk_push_object(ctx);

    if (!duk_is_string(ctx, 0)) { return 1; }
    String filepath = duk_to_string(ctx, 0);

    if (filepath == "audio") {
        putPropAudioFunctions(ctx, obj_idx, 0);
    } else if (filepath == "badusb") {
        putPropBadUSBFunctions(ctx, obj_idx, 0);
    } else if (filepath == "blebeacon") {

    } else if (filepath == "dialog" || filepath == "gui") {
        putPropDialogFunctions(ctx, obj_idx, 0);
    } else if (filepath == "display") {
        putPropDisplayFunctions(ctx, obj_idx, 0);

    } else if (filepath == "device" || filepath == "flipper") {
        putPropDeviceFunctions(ctx, obj_idx, 0);
    } else if (filepath == "gpio") {
        putPropGPIOFunctions(ctx, obj_idx, 0);
    } else if (filepath == "i2c") {
        putPropI2CFunctions(ctx, obj_idx, 0);
    } else if (filepath == "http") {
        // TODO: Make the WebServer API compatible with the Node.js API
        // The more compatible we are, the more Node.js scripts can run on Bruce
        // MEMO: We need to implement an event loop so the WebServer can run:
        // https://github.com/svaarala/duktape/tree/master/examples/eventloop

    } else if (filepath == "ir") {
        putPropIRFunctions(ctx, obj_idx, 0);
    } else if (filepath == "keyboard" || filepath == "input") {
        putPropKeyboardFunctions(ctx, obj_idx, 0);
    } else if (filepath == "math") {
        putPropMathFunctions(ctx, obj_idx, 0);
    } else if (filepath == "notification") {
        putPropNotificationFunctions(ctx, obj_idx, 0);
    } else if (filepath == "serial") {
        putPropSerialFunctions(ctx, obj_idx, 0);
    } else if (filepath == "storage") {
        putPropStorageFunctions(ctx, obj_idx, 0);
    } else if (filepath == "subghz") {
        putPropSubGHzFunctions(ctx, obj_idx, 0);
    } else if (filepath == "wifi") {
        putPropWiFiFunctions(ctx, obj_idx, 0);
    } else {
        FS *fs = NULL;
        if (SD.exists(filepath)) fs = &SD;
        else if (LittleFS.exists(filepath)) fs = &LittleFS;
        if (fs == NULL) { return 1; }

        const char *requiredScript = readBigFile(*fs, filepath);
        if (requiredScript == NULL) { return 1; }

        duk_push_string(ctx, "(function(){exports={};module={exports:exports};\n");
        duk_push_string(ctx, requiredScript);
        duk_push_string(ctx, "\n})");
        duk_concat(ctx, 3);

        duk_int_t pcall_rc = duk_pcompile(ctx, DUK_COMPILE_EVAL);
        if (pcall_rc != DUK_EXEC_SUCCESS) { return 1; }

        pcall_rc = duk_pcall(ctx, 1);
        if (pcall_rc == DUK_EXEC_SUCCESS) {
            duk_get_prop_string(ctx, -1, "exports");
            duk_compact(ctx, -1);
        }
    }

    return 1;
}
*/

#endif
