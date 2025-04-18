#include "interpreter.h"

#include "display_js.h"
#include "functions_js.h"
#include "gui_js.h"
#include "helpers_js.h"
#include "wifi_js.h"

static char *script = NULL;
static char *scriptDirpath = NULL;
static char *scriptName = NULL;

static duk_ret_t native_load(duk_context *ctx) {
    free((char *)script);
    free((char *)scriptDirpath);
    free((char *)scriptName);
    script = strdup(duk_to_string(ctx, 0));
    scriptDirpath = NULL;
    scriptName = NULL;
    return 0;
}

static void registerConsole(duk_context *ctx) {
    duk_idx_t obj_idx = duk_push_object(ctx);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "log", native_serialPrintln, DUK_VARARGS);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "debug", native_serialPrintln, DUK_VARARGS, 2);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "warn", native_serialPrintln, DUK_VARARGS, 3);
    bduk_put_prop_c_lightfunc(ctx, obj_idx, "error", native_serialPrintln, DUK_VARARGS, 4);

    duk_put_global_string(ctx, "console");
}

static const char *nth_strchr(const char *s, char c, int8_t n) {
    const char *nth = s;
    if (c == '\0' || n < 1) return NULL;

    for (int i = 0; i < n; i++) {
        if ((nth = strchr(nth, c)) == 0) break;
        nth++;
    }

    return nth;
}

static void *ps_alloc_function(void *udata, duk_size_t size) {
    void *res;
    DUK_UNREF(udata);
    res = ps_malloc(size);
    return res;
}

static void *ps_realloc_function(void *udata, void *ptr, duk_size_t newsize) {
    void *res;
    DUK_UNREF(udata);
    res = ps_realloc(ptr, newsize);
    return res;
}

static void ps_free_function(void *udata, void *ptr) {
    DUK_UNREF(udata);
    DUK_ANSI_FREE(ptr);
}

static void js_fatal_error_handler(void *udata, const char *msg) {
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
    while (!check(AnyKeyPress));
    // We need to restart esp32 after fatal error
    abort();
}

InterpreterJS::InterpreterJS(int id, const char *script) {}

InterpreterJS::~InterpreterJS() {}

// Code interpreter, must be called in the loop() function to work
void interpreterHandler(void *pvParameters) {
    log_d(
        "init interpreter:\nPSRAM: [Free: %d, max alloc: %d],\nRAM: [Free: %d, "
        "max alloc: %d]\n",
        ESP.getFreePsram(),
        ESP.getMaxAllocPsram(),
        ESP.getFreeHeap(),
        ESP.getMaxAllocHeap()
    );
    if (script == NULL) { return; }
    tft.fillScreen(TFT_BLACK);
    tft.setRotation(bruceConfig.rotation);
    tft.setTextSize(FM);
    tft.setTextColor(TFT_WHITE);
    // Create context.
    Serial.println("Create context");
    auto alloc_function = &ps_alloc_function;
    auto realloc_function = &ps_realloc_function;
    auto free_function = &ps_free_function;
    if (!psramFound()) {
        alloc_function = NULL;
        realloc_function = NULL;
        free_function = NULL;
    }

    /// TODO: Add DUK_USE_NATIVE_STACK_CHECK check with
    /// uxTaskGetStackHighWaterMark
    duk_context *ctx =
        duk_create_heap(alloc_function, realloc_function, free_function, NULL, js_fatal_error_handler);

    // Add native functions to context.
    bduk_register_c_lightfunc(ctx, "now", native_now, 0);
    bduk_register_c_lightfunc(ctx, "delay", native_delay, 1);
    bduk_register_c_lightfunc(ctx, "parse_int", native_parse_int, 1);
    bduk_register_c_lightfunc(ctx, "to_string", native_to_string, 1);
    bduk_register_c_lightfunc(ctx, "to_hex_string", native_to_hex_string, 1);
    bduk_register_c_lightfunc(ctx, "to_lower_case", native_to_lower_case, 1);
    bduk_register_c_lightfunc(ctx, "to_upper_case", native_to_upper_case, 1);
    bduk_register_c_lightfunc(ctx, "random", native_random, 2);
    bduk_register_c_lightfunc(ctx, "require", native_require, 1);
    bduk_register_c_lightfunc(ctx, "assert", native_assert, 2);
    if (scriptDirpath == NULL || scriptName == NULL) {
        bduk_register_string(ctx, "__filepath", "");
        bduk_register_string(ctx, "__dirpath", "");
    } else {
        bduk_register_string(ctx, "__filepath", (String(scriptDirpath) + String(scriptName)).c_str());
        bduk_register_string(ctx, "__dirpath", scriptDirpath);
    }
    bduk_register_string(ctx, "BRUCE_VERSION", BRUCE_VERSION);
    bduk_register_int(ctx, "BRUCE_PRICOLOR", bruceConfig.priColor);
    bduk_register_int(ctx, "BRUCE_SECCOLOR", bruceConfig.secColor);
    bduk_register_int(ctx, "BRUCE_BGCOLOR", bruceConfig.bgColor);

    registerConsole(ctx);

    // Typescript emits: Object.defineProperty(exports, "__esModule", { value:
    // true }); In every file, this is polyfill so typescript project can run on
    // Bruce
    duk_push_object(ctx);
    duk_put_global_string(ctx, "exports");

    // Arduino compatible
    bduk_register_c_lightfunc(ctx, "pinMode", native_pinMode, 2);
    bduk_register_c_lightfunc(ctx, "digitalWrite", native_digitalWrite, 2);
    bduk_register_c_lightfunc(ctx, "analogWrite", native_analogWrite, 2);
    bduk_register_c_lightfunc(ctx, "dacWrite", native_dacWrite,
                              2); // only pins 25 and 26
    bduk_register_c_lightfunc(ctx, "digitalRead", native_digitalRead, 1);
    bduk_register_c_lightfunc(ctx, "analogRead", native_analogRead, 1);
    bduk_register_c_lightfunc(ctx, "touchRead", native_touchRead, 1);
    bduk_register_int(ctx, "HIGH", HIGH);
    bduk_register_int(ctx, "LOW", LOW);
    bduk_register_int(ctx, "INPUT", INPUT);
    bduk_register_int(ctx, "OUTPUT", OUTPUT);
    bduk_register_int(ctx, "PULLUP", PULLUP);
    bduk_register_int(ctx, "INPUT_PULLUP", INPUT_PULLUP);
    bduk_register_int(ctx, "PULLDOWN", PULLDOWN);
    bduk_register_int(ctx, "INPUT_PULLDOWN", INPUT_PULLDOWN);

    // Deprecated
    bduk_register_c_lightfunc(ctx, "load", native_load, 1);

    // Get Informations from the board
    bduk_register_c_lightfunc(ctx, "getBattery", native_getBattery, 0);
    bduk_register_c_lightfunc(ctx, "getBoard", native_getBoard, 0);
    bduk_register_c_lightfunc(ctx, "getFreeHeapSize", native_getFreeHeapSize, 0);

    // Networking
    bduk_register_c_lightfunc(ctx, "wifiConnect", native_wifiConnect, 3);
    bduk_register_c_lightfunc(ctx, "wifiConnectDialog", native_wifiConnectDialog, 0);
    bduk_register_c_lightfunc(ctx, "wifiDisconnect", native_wifiDisconnect, 0);
    bduk_register_c_lightfunc(ctx, "wifiScan", native_wifiScan, 0);

    bduk_register_c_lightfunc(ctx, "httpFetch", native_httpFetch, 2, 0);
    bduk_register_c_lightfunc(ctx, "httpGet", native_httpFetch, 2, 0);

    // Graphics
    bduk_register_c_lightfunc(ctx, "color", native_color, 4);
    bduk_register_c_lightfunc(ctx, "fillScreen", native_fillScreen, 1);
    bduk_register_c_lightfunc(ctx, "setTextColor", native_setTextColor, 1);
    bduk_register_c_lightfunc(ctx, "setTextSize", native_setTextSize, 1);
    bduk_register_c_lightfunc(ctx, "drawString", native_drawString, 3);
    bduk_register_c_lightfunc(ctx, "setCursor", native_setCursor, 2);
    bduk_register_c_lightfunc(ctx, "print", native_print, DUK_VARARGS);
    bduk_register_c_lightfunc(ctx, "println", native_println, DUK_VARARGS);
    bduk_register_c_lightfunc(ctx, "drawPixel", native_drawPixel, 3);
    bduk_register_c_lightfunc(ctx, "drawLine", native_drawLine, 5);
    bduk_register_c_lightfunc(ctx, "drawRect", native_drawRect, 5);
    bduk_register_c_lightfunc(ctx, "drawFillRect", native_drawFillRect, 5);
    // bduk_register_c_lightfunc(ctx, "drawBitmap", native_drawBitmap, 4);
    bduk_register_c_lightfunc(ctx, "drawJpg", native_drawJpg, 4);
    bduk_register_c_lightfunc(ctx, "drawGif", native_drawGif, 6);
    bduk_register_c_lightfunc(ctx, "gifOpen", native_gifOpen, 2);
    bduk_register_c_lightfunc(ctx, "width", native_width, 0);
    bduk_register_c_lightfunc(ctx, "height", native_height, 0);

    // Input
    bduk_register_c_lightfunc(
        ctx,
        "getKeysPressed",
        native_getKeysPressed,
        0
    ); // keyboard btns for cardputer (entry)
    bduk_register_c_lightfunc(ctx, "getPrevPress", native_getPrevPress, 0);
    bduk_register_c_lightfunc(ctx, "getSelPress", native_getSelPress, 0);
    bduk_register_c_lightfunc(ctx, "getEscPress", native_getEscPress, 0);
    bduk_register_c_lightfunc(ctx, "getNextPress", native_getNextPress, 0);
    bduk_register_c_lightfunc(ctx, "getAnyPress", native_getAnyPress, 0);

    // Serial
    bduk_register_c_lightfunc(ctx, "serialReadln", native_serialReadln, 1);
    bduk_register_c_lightfunc(ctx, "serialPrintln", native_serialPrintln, DUK_VARARGS);
    bduk_register_c_lightfunc(ctx, "serialCmd", native_serialCmd, 1);

    // Audio
    bduk_register_c_lightfunc(ctx, "playAudioFile", native_playAudioFile, 1);
    bduk_register_c_lightfunc(ctx, "tone", native_tone, 3);

    // badusb
    bduk_register_c_lightfunc(ctx, "badusbSetup", native_badusbSetup, 0);
    bduk_register_c_lightfunc(ctx, "badusbPrint", native_badusbPrint, 1);
    bduk_register_c_lightfunc(ctx, "badusbPrintln", native_badusbPrintln, 1);
    bduk_register_c_lightfunc(ctx, "badusbPress", native_badusbPress, 1);
    bduk_register_c_lightfunc(ctx, "badusbHold", native_badusbHold, 1);
    bduk_register_c_lightfunc(ctx, "badusbRelease", native_badusbRelease, 1);
    bduk_register_c_lightfunc(ctx, "badusbReleaseAll", native_badusbReleaseAll, 0);
    bduk_register_c_lightfunc(ctx, "badusbPressRaw", native_badusbPressRaw, 1);
    bduk_register_c_lightfunc(ctx, "badusbRunFile", native_badusbRunFile, 1);
    // bduk_register_c_lightfunc(ctx, "badusbPressSpecial",
    // native_badusbPressSpecial, 1);

    // IR
    bduk_register_c_lightfunc(ctx, "irRead", native_irRead, 1);
    bduk_register_c_lightfunc(ctx, "irReadRaw", native_irRead, 1, 1);
    bduk_register_c_lightfunc(ctx, "irTransmitFile", native_irTransmitFile, 1);
    // TODO: irTransmit(string)

    // subghz
    bduk_register_c_lightfunc(ctx, "subghzRead", native_subghzRead, 0);
    bduk_register_c_lightfunc(ctx, "subghzReadRaw", native_subghzReadRaw, 0);
    bduk_register_c_lightfunc(ctx, "subghzSetFrequency", native_subghzSetFrequency, 1);
    bduk_register_c_lightfunc(ctx, "subghzTransmitFile", native_subghzTransmitFile, 1);
    // bduk_register_c_lightfunc(ctx, "subghzSetIdle", native_subghzSetIdle, 1);
    // TODO: subghzTransmit(string)

    // Dialog functions
    bduk_register_c_lightfunc(ctx, "dialogMessage", native_dialogNotification, 2, 0);
    bduk_register_c_lightfunc(ctx, "dialogError", native_dialogNotification, 2, 3);
    bduk_register_c_lightfunc(ctx, "dialogChoice", native_dialogChoice, 1, 1);
    bduk_register_c_lightfunc(ctx, "dialogPickFile", native_dialogPickFile, 2);
    bduk_register_c_lightfunc(ctx, "dialogViewFile", native_dialogViewFile, 1);
    bduk_register_c_lightfunc(ctx, "keyboard", native_keyboard, 3);

    // Storage
    bduk_register_c_lightfunc(ctx, "storageReaddir", native_storageReaddir, 1);
    bduk_register_c_lightfunc(ctx, "storageRead", native_storageRead, 2);
    bduk_register_c_lightfunc(ctx, "storageWrite", native_storageWrite, 4);
    bduk_register_c_lightfunc(ctx, "storageRename", native_storageRename, 2);
    bduk_register_c_lightfunc(ctx, "storageRemove", native_storageRemove, 1);

    log_d(
        "global populated:\nPSRAM: [Free: %d, max alloc: %d],\nRAM: [Free: %d, "
        "max alloc: %d]\n",
        ESP.getFreePsram(),
        ESP.getMaxAllocPsram(),
        ESP.getFreeHeap(),
        ESP.getMaxAllocHeap()
    );

    // TODO: match flipper syntax
    // https://github.com/jamisonderek/flipper-zero-tutorials/wiki/JavaScript
    // MEMO: API https://duktape.org/api.html
    // https://github.com/joeqread/arduino-duktape/blob/main/src/duktape.h

    Serial.printf("Script length: %d\n", strlen(script));

    if (duk_peval_string(ctx, script) != DUK_EXEC_SUCCESS) {
        tft.fillScreen(bruceConfig.bgColor);
        tft.setTextSize(FM);
        tft.setTextColor(TFT_RED, bruceConfig.bgColor);
        tft.drawCentreString("Error", tftWidth / 2, 10, 1);
        tft.setTextColor(TFT_WHITE, bruceConfig.bgColor);
        tft.setTextSize(FP);
        tft.setCursor(0, 33);

        String errorMessage = "";
        if (duk_is_error(ctx, -1)) {
            errorMessage = duk_safe_to_stacktrace(ctx, -1);
        } else {
            errorMessage = duk_safe_to_string(ctx, -1);
        }
        Serial.printf("eval failed: %s\n", errorMessage.c_str());
        tft.printf("%s\n\n", errorMessage.c_str());

        int lineIndexOf = errorMessage.indexOf("line ");
        int evalIndexOf = errorMessage.indexOf("(eval:");
        Serial.printf("lineIndexOf: %d\n", lineIndexOf);
        Serial.printf("evalIndexOf: %d\n", evalIndexOf);
        String errorLine = "";
        if (lineIndexOf != -1) {
            lineIndexOf += 5;
            errorLine = errorMessage.substring(lineIndexOf, errorMessage.indexOf("\n", lineIndexOf));
        } else if (evalIndexOf != -1) {
            evalIndexOf += 6;
            errorLine = errorMessage.substring(evalIndexOf, errorMessage.indexOf(")", evalIndexOf));
        }
        Serial.printf("errorLine: [%s]\n", errorLine.c_str());

        if (errorLine != "") {
            uint8_t errorLineNumber = errorLine.toInt();
            const char *errorScript = nth_strchr(script, '\n', errorLineNumber - 1);
            Serial.printf("%.80s\n\n", errorScript);
            tft.printf("%.80s\n\n", errorScript);

            if (strstr(errorScript, "let ")) {
                Serial.println("let is not supported, change it to var");
                tft.println("let is not supported, change it to var");
            }
        }

        delay(500);
        while (!check(AnyKeyPress)) { delay(50); }
    } else {
        duk_uint_t resultType = duk_get_type_mask(ctx, -1);
        if (resultType & (DUK_TYPE_MASK_STRING | DUK_TYPE_MASK_NUMBER)) {
            printf("Script ran succesfully, result is: %s\n", duk_safe_to_string(ctx, -1));
        } else {
            printf("Script ran succesfully");
        }
    }
    free((char *)script);
    script = NULL;
    free((char *)scriptDirpath);
    scriptDirpath = NULL;
    free((char *)scriptName);
    scriptName = NULL;
    duk_pop(ctx);

    // Clean up.
    duk_destroy_heap(ctx);

    // delay(1000);
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
    scriptDirpath = NULL;
    scriptName = NULL;
    returnToMenu = true;
    interpreter_start = true;
    return true;
}

bool run_bjs_script_headless(FS fs, String filename) {
    script = readBigFile(fs, filename);
    if (script == NULL) { return false; }
    const char *sName = filename.substring(0, filename.lastIndexOf('/')).c_str();
    const char *sDirpath = filename.substring(filename.lastIndexOf('/') + 1).c_str();
    scriptDirpath = strdup(sDirpath);
    scriptName = strdup(sName);
    returnToMenu = true;
    interpreter_start = true;
    return true;
}
