#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "display_js.h"

#include "core/settings.h"
#include "helpers_js.h"
#include "stdio.h"
#include "user_classes_js.h"

JSValue native_color(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int r = 0, g = 0, b = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &r, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &g, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &b, argv[2]);
    int color = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    int mode = 16;
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &mode, argv[3]);
    if (mode == 16) {
        return JS_NewInt32(ctx, color);
    } else {
        return JS_NewInt32(ctx, ((color & 0xE000) >> 8) | ((color & 0x0700) >> 6) | ((color & 0x0018) >> 3));
    }
}

static std::vector<TFT_eSprite *> sprites;
typedef struct {
    TFT_eSprite *sprite;
} SpriteData;

void clearSpritesVector() {
    for (auto s : sprites) {
        if (s) {
            delete s;
            s = NULL;
        }
    }
    sprites.clear();
}

/* Finalizer called by mquickjs when a Sprite JS object is freed. */
void native_sprite_finalizer(JSContext *ctx, void *opaque) {
    SpriteData *d = (SpriteData *)opaque;
    if (!d) return;
#if defined(HAS_SCREEN)
    if (d->sprite) {
        TFT_eSprite *p = d->sprite;
        // clear any entry in the numeric sprites table
        for (size_t i = 0; i < sprites.size(); ++i) {
            if (sprites[i] == p) {
                sprites[i] = NULL;
                break;
            }
        }
        delete p;
        d->sprite = NULL;
    }
#endif
    free(d);
}

#if defined(HAS_SCREEN)
static inline TFT_eSPI *get_display(JSContext *ctx, JSValue *this_val) __attribute__((always_inline));
static inline TFT_eSPI *get_display(JSContext *ctx, JSValue *this_val) {
    if (this_val && JS_IsObject(ctx, *this_val)) {
        JSValue v = JS_GetPropertyStr(ctx, *this_val, "spritePointer");
        if (!JS_IsUndefined(v)) {
            int tmp;
            JS_ToInt32(ctx, &tmp, v);
            int index = tmp - 1;
            if (index >= 0 && index < (int)sprites.size() && sprites[index]) {
                return reinterpret_cast<TFT_eSPI *>(sprites[index]);
            }
        }
    }
    return reinterpret_cast<TFT_eSPI *>(&tft);
}
#else
static inline SerialDisplayClass *get_display(JSContext *ctx, JSValue *this_val)
    __attribute__((always_inline));
static inline SerialDisplayClass *get_display(JSContext *ctx, JSValue *this_val) {
    return static_cast<SerialDisplayClass *>(&tft);
}
#endif

JSValue native_setTextColor(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int c = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &c, argv[0]);
    get_display(ctx, this_val)->setTextColor(c);
    return JS_UNDEFINED;
}

JSValue native_setTextSize(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int s = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &s, argv[0]);
    get_display(ctx, this_val)->setTextSize(s);
    return JS_UNDEFINED;
}

JSValue native_setTextAlign(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    uint8_t align = 0;
    uint8_t baseline = 0;
    if (argc > 0) {
        if (JS_IsString(ctx, argv[0])) {
            JSCStringBuf sb;
            const char *s = JS_ToCString(ctx, argv[0], &sb);
            if (s) {
                if (s[0] == 'l') align = 0;
                else if (s[0] == 'c') align = 1;
                else if (s[0] == 'r') align = 2;
            }
        } else if (JS_IsNumber(ctx, argv[0])) {
            JS_ToInt32(ctx, (int *)&align, argv[0]);
        }
    }
    if (argc > 1) {
        if (JS_IsString(ctx, argv[1])) {
            JSCStringBuf sb;
            const char *s = JS_ToCString(ctx, argv[1], &sb);
            if (s) {
                if (s[0] == 't') baseline = 0;
                else if (s[0] == 'm') baseline = 1;
                else if (s[0] == 'b') baseline = 2;
                else if (s[0] == 'a') baseline = 3;
            }
        } else if (JS_IsNumber(ctx, argv[1])) {
            JS_ToInt32(ctx, (int *)&baseline, argv[1]);
        }
    }
    get_display(ctx, this_val)->setTextDatum(align + baseline * 3);
    return JS_UNDEFINED;
}

JSValue native_drawRect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int a = 0, b = 0, c = 0, d = 0, e = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &d, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &e, argv[4]);
    get_display(ctx, this_val)->drawRect(a, b, c, d, e);
    return JS_UNDEFINED;
}

JSValue native_drawFillRect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int a = 0, b = 0, c = 0, d = 0, e = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &d, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &e, argv[4]);
    get_display(ctx, this_val)->fillRect(a, b, c, d, e);
    return JS_UNDEFINED;
}

JSValue native_drawFillRectGradient(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(HAS_SCREEN)
    char mode = 'h';
    if (argc > 6 && JS_IsString(ctx, argv[6])) {
        JSCStringBuf sb;
        const char *s = JS_ToCString(ctx, argv[6], &sb);
        if (s) mode = s[0];
    }
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &d, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &e, argv[4]);
    if (argc > 5 && JS_IsNumber(ctx, argv[5])) JS_ToInt32(ctx, &f, argv[5]);
    if (mode == 'h') {
        get_display(ctx, this_val)->fillRectHGradient(a, b, c, d, e, f);
    } else {
        get_display(ctx, this_val)->fillRectVGradient(a, b, c, d, e, f);
    }
#else
    int sprite = get_sprite_index(ctx, this_val, argc, argv);
    int a = 0, b = 0, c = 0, d = 0, e = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &d, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &e, argv[4]);
    get_display(ctx, this_val)->fillRect(a, b, c, d, e);
#endif
    return JS_UNDEFINED;
}

JSValue native_drawRoundRect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &d, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &e, argv[4]);
    if (argc > 5 && JS_IsNumber(ctx, argv[5])) JS_ToInt32(ctx, &f, argv[5]);
    get_display(ctx, this_val)->drawRoundRect(a, b, c, d, e, f);
    return JS_UNDEFINED;
}

JSValue native_drawFillRoundRect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &d, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &e, argv[4]);
    if (argc > 5 && JS_IsNumber(ctx, argv[5])) JS_ToInt32(ctx, &f, argv[5]);
    get_display(ctx, this_val)->fillRoundRect(a, b, c, d, e, f);
    return JS_UNDEFINED;
}

JSValue native_drawCircle(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int a = 0, b = 0, c = 0, d = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &d, argv[3]);
    get_display(ctx, this_val)->drawCircle(a, b, c, d);
    return JS_UNDEFINED;
}

JSValue native_drawFillCircle(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int a = 0, b = 0, c = 0, d = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &d, argv[3]);
    get_display(ctx, this_val)->fillCircle(a, b, c, d);
    return JS_UNDEFINED;
}

JSValue native_drawLine(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int a = 0, b = 0, c = 0, d = 0, e = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &d, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &e, argv[4]);
    get_display(ctx, this_val)->drawLine(a, b, c, d, e);
    return JS_UNDEFINED;
}

JSValue native_drawPixel(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int a = 0, b = 0, c = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    get_display(ctx, this_val)->drawPixel(a, b, c);
    return JS_UNDEFINED;
}

JSValue native_drawXBitmap(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // Accept strings, Uint8Array, ArrayBuffer or JS arrays containing byte values.
    int bitmapWidth = 0, bitmapHeight = 0;
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &bitmapWidth, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &bitmapHeight, argv[4]);

    size_t len = 0;
    const uint8_t *data = NULL;

    if (argc > 2) {
        const char *s = NULL;
        if (JS_IsString(ctx, argv[2])) {
            JSCStringBuf sb;
            s = JS_ToCStringLen(ctx, &len, argv[2], &sb);
        } else if (JS_IsTypedArray(ctx, argv[2])) {
            s = JS_GetTypedArrayBuffer(ctx, &len, argv[2]);
        }
        if (s) { data = (const uint8_t *)s; }
    }

    if (data == NULL) {
        return JS_ThrowTypeError(
            ctx, "%s: Expected string/ArrayBuffer/Uint8Array for bitmap data", "drawXBitmap"
        );
    }

    size_t expectedSize = ((bitmapWidth + 7) / 8) * bitmapHeight;
    if (len != expectedSize) {
        Serial.printf("data = %s\n", data);
        return JS_ThrowTypeError(ctx, "Bitmap size mismatch, len: %d, expected: %d", len, expectedSize);
    }

    int x = 0, y = 0, fg = 0, bg = -1;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    if (argc > 5 && JS_IsNumber(ctx, argv[5])) JS_ToInt32(ctx, &fg, argv[5]);
    if (argc > 6 && JS_IsNumber(ctx, argv[6])) JS_ToInt32(ctx, &bg, argv[6]);

    if (bg >= 0) {
        get_display(ctx, this_val)->drawXBitmap(x, y, (uint8_t *)data, bitmapWidth, bitmapHeight, fg, bg);
    } else {
        get_display(ctx, this_val)->drawXBitmap(x, y, (uint8_t *)data, bitmapWidth, bitmapHeight, fg);
    }

    return JS_UNDEFINED;
}

JSValue native_drawString(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc > 0 && JS_IsString(ctx, argv[0])) {
        JSCStringBuf sb;
        const char *s = JS_ToCString(ctx, argv[0], &sb);
        int x = 0, y = 0;
        if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &x, argv[1]);
        if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &y, argv[2]);
        get_display(ctx, this_val)->drawString(s, x, y);
    }
    return JS_UNDEFINED;
}

JSValue native_setCursor(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int x = 0, y = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    get_display(ctx, this_val)->setCursor(x, y);
    return JS_UNDEFINED;
}

JSValue native_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    internal_print(ctx, this_val, argc, argv, true, false);
    return JS_UNDEFINED;
}

JSValue native_println(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    internal_print(ctx, this_val, argc, argv, true, true);
    return JS_UNDEFINED;
}

JSValue native_fillScreen(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int c = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &c, argv[0]);
    if (this_val && JS_IsObject(ctx, *this_val)) {
        JSValue v = JS_GetPropertyStr(ctx, *this_val, "spritePointer");
        if (!JS_IsUndefined(v)) {
            reinterpret_cast<TFT_eSprite *>(get_display(ctx, this_val))->fillSprite(c);
            return JS_UNDEFINED;
        }
    }
    tft.fillScreen(c);
    return JS_UNDEFINED;
}

JSValue native_width(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int width = 0;
    width = get_display(ctx, this_val)->width();
    return JS_NewInt32(ctx, width);
}

JSValue native_height(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int height = 0;
    height = get_display(ctx, this_val)->height();
    return JS_NewInt32(ctx, height);
}

JSValue native_drawImage(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    FileParamsJS file = js_get_path_from_params(ctx, argv, true, true);
    drawImg(*file.fs, file.path, 0, 0, 0);
    return JS_UNDEFINED;
}

JSValue native_drawJpg(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    FileParamsJS file = js_get_path_from_params(ctx, argv, true, true);
    showJpeg(*file.fs, file.path, 0, 0, 0);
    return JS_UNDEFINED;
}

#if !defined(LITE_VERSION)
JSValue native_drawGif(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    (void)this_val;
    FileParamsJS file = js_get_path_from_params(ctx, argv, true, true);

    int x = 0, y = 0, center = 0, playDurationMs = 0;
    int base = file.paramOffset;
    if (argc > base && JS_IsNumber(ctx, argv[base])) JS_ToInt32(ctx, &x, argv[base]);
    if (argc > base + 1 && JS_IsNumber(ctx, argv[base + 1])) JS_ToInt32(ctx, &y, argv[base + 1]);
    if (argc > base + 2) {
        if (JS_IsBool(argv[base + 2])) center = JS_ToBool(ctx, argv[base + 2]);
        else if (JS_IsNumber(ctx, argv[base + 2])) JS_ToInt32(ctx, &center, argv[base + 2]);
    }
    if (argc > base + 3 && JS_IsNumber(ctx, argv[base + 3])) JS_ToInt32(ctx, &playDurationMs, argv[base + 3]);

    showGif(file.fs, file.path.c_str(), x, y, center != 0, playDurationMs);
    return JS_UNDEFINED;
}

static std::vector<Gif *> gifs;
void clearGifsVector() {
    for (auto gif : gifs) {
        delete gif;
        gif = NULL;
    }
    gifs.clear();
}

JSValue native_gifPlayFrame(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int gifIndex = 0;
    int x = 0, y = 0, bSync = 1;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &bSync, argv[2]);

    // try this object first
    if (this_val && JS_IsObject(ctx, *this_val)) {
        JSValue v = JS_GetPropertyStr(ctx, *this_val, "gifPointer");
        if (!JS_IsUndefined(v)) {
            int tmp;
            JS_ToInt32(ctx, &tmp, v);
            gifIndex = tmp - 1;
        }
    }

    uint8_t result = 0;
    if (gifIndex >= 0 && gifIndex < (int)gifs.size()) {
        Gif *gif = gifs.at(gifIndex);
        if (gif != NULL) { result = gif->playFrame(x, y, bSync); }
    }
    return JS_NewInt32(ctx, result);
}

JSValue native_gifDimensions(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int gifIndex = -1;
    if (this_val && JS_IsObject(ctx, *this_val)) {
        JSValue v = JS_GetPropertyStr(ctx, *this_val, "gifPointer");
        if (!JS_IsUndefined(v)) {
            int tmp;
            JS_ToInt32(ctx, &tmp, v);
            gifIndex = tmp - 1;
        }
    }
    if (gifIndex < 0 || gifIndex >= (int)gifs.size()) { return JS_NewInt32(ctx, 0); }
    Gif *gif = gifs.at(gifIndex);
    if (!gif) return JS_NewInt32(ctx, 0);
    int canvasWidth = gif->getCanvasWidth();
    int canvasHeight = gif->getCanvasHeight();
    JSValue obj = JS_NewObject(ctx);
    JS_SetPropertyStr(ctx, obj, "width", JS_NewInt32(ctx, canvasWidth));
    JS_SetPropertyStr(ctx, obj, "height", JS_NewInt32(ctx, canvasHeight));
    return obj;
}

JSValue native_gifReset(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int gifIndex = -1;
    if (this_val && JS_IsObject(ctx, *this_val)) {
        JSValue v = JS_GetPropertyStr(ctx, *this_val, "gifPointer");
        if (!JS_IsUndefined(v)) {
            int tmp;
            JS_ToInt32(ctx, &tmp, v);
            gifIndex = tmp - 1;
        }
    }
    uint8_t result = 0;
    if (gifIndex >= 0 && gifIndex < (int)gifs.size()) {
        Gif *gif = gifs.at(gifIndex);
        if (gif != NULL) {
            gifs.at(gifIndex)->reset();
            result = 1;
        }
    }
    return JS_NewInt32(ctx, result);
}

JSValue native_gifClose(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int gifIndex = -1;
    if (argc > 0 && JS_IsObject(ctx, argv[0])) {
        JSValue v = JS_GetPropertyStr(ctx, argv[0], "gifPointer");
        if (!JS_IsUndefined(v)) {
            int tmp;
            JS_ToInt32(ctx, &tmp, v);
            gifIndex = tmp - 1;
        }
    } else if (this_val && JS_IsObject(ctx, *this_val)) {
        JSValue v = JS_GetPropertyStr(ctx, *this_val, "gifPointer");
        if (!JS_IsUndefined(v)) {
            int tmp;
            JS_ToInt32(ctx, &tmp, v);
            gifIndex = tmp - 1;
        }
    }
    uint8_t result = 0;
    if (gifIndex >= 0 && gifIndex < (int)gifs.size()) {
        Gif *gif = gifs.at(gifIndex);
        if (gif != NULL) {
            delete gif;
            gifs.at(gifIndex) = NULL;
            result = 1;
            if (this_val && JS_IsObject(ctx, *this_val))
                JS_SetPropertyStr(ctx, *this_val, "gifPointer", JS_NewInt32(ctx, 0));
        }
    }
    return JS_NewInt32(ctx, result);
}

JSValue native_gifOpen(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    FileParamsJS file = js_get_path_from_params(ctx, argv, true, true);
    Gif *gif = new Gif();
    bool success = gif->openGIF(file.fs, file.path.c_str());
    if (!success) {
        return JS_NULL;
    } else {
        gifs.push_back(gif);
        JSValue obj = JS_NewObject(ctx);
        JS_SetPropertyStr(ctx, obj, "gifPointer", JS_NewInt32(ctx, gifs.size()));

        JSValue global = JS_GetGlobalObject(ctx);
        JSValue internalFunctions = JS_GetPropertyStr(ctx, global, "internal_functions");
        if (!JS_IsUndefined(internalFunctions)) {
            JSValue f;
            f = JS_GetPropertyStr(ctx, internalFunctions, "gifPlayFrame");
            if (!JS_IsUndefined(f)) JS_SetPropertyStr(ctx, obj, "playFrame", f);

            f = JS_GetPropertyStr(ctx, internalFunctions, "gifDimensions");
            if (!JS_IsUndefined(f)) JS_SetPropertyStr(ctx, obj, "dimensions", f);

            f = JS_GetPropertyStr(ctx, internalFunctions, "gifReset");
            if (!JS_IsUndefined(f)) JS_SetPropertyStr(ctx, obj, "reset", f);

            f = JS_GetPropertyStr(ctx, internalFunctions, "gifClose");
            if (!JS_IsUndefined(f)) JS_SetPropertyStr(ctx, obj, "close", f);
        }

        return obj;
    }
}
#endif

void clearDisplayModuleData() {
    clearGifsVector();
    clearSpritesVector();
}

JSValue native_deleteSprite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    JSValue obj = JS_UNDEFINED;
    if (argc > 0 && JS_IsObject(ctx, argv[0])) obj = argv[0];
    else if (this_val && JS_IsObject(ctx, *this_val)) obj = *this_val;
    if (JS_IsUndefined(obj)) return JS_NewInt32(ctx, 0);

    /* Try numeric spritePointer first (plain objects created by native_createSprite)
       which store a 1-based index. If found, delete the underlying C sprite
       and clear the property. Otherwise fall back to class/opaque-based deletion. */
    JSValue v = JS_GetPropertyStr(ctx, obj, "spritePointer");
    if (!JS_IsUndefined(v)) {
        int tmp;
        JS_ToInt32(ctx, &tmp, v);
        int idx = tmp - 1;
        if (idx >= 0 && idx < (int)sprites.size() && sprites[idx]) {
            delete sprites[idx];
            sprites[idx] = NULL;
            JS_SetPropertyStr(ctx, obj, "spritePointer", JS_NewInt32(ctx, 0));
            return JS_NewInt32(ctx, 1);
        }
        return JS_NewInt32(ctx, 0);
    }

    int cid = JS_GetClassID(ctx, obj);
    if (cid != JS_CLASS_SPRITE) return JS_NewInt32(ctx, 0);
    void *opaque = JS_GetOpaque(ctx, obj);
    if (!opaque) return JS_NewInt32(ctx, 0);
    // Call finalizer to free underlying C resources, then clear opaque
    native_sprite_finalizer(ctx, opaque);
    JS_SetOpaque(ctx, obj, NULL);
    return JS_NewInt32(ctx, 1);
}

JSValue native_pushSprite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(HAS_SCREEN)
#ifndef BOARD_HAS_PSRAM
    return JS_NewInt32(ctx, 1);
#else
    JSValue v = JS_GetPropertyStr(ctx, *this_val, "spritePointer");
    if (!JS_IsUndefined(v)) {
        reinterpret_cast<TFT_eSprite *>(get_display(ctx, this_val))->pushSprite(0, 0);
        return JS_UNDEFINED;
    }
#endif
#endif
    return JS_UNDEFINED;
}

JSValue native_createSprite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(HAS_SCREEN)
    int16_t width = tft.width();
    int16_t height = tft.height();
    uint8_t colorDepth = 16;
    uint8_t frames = 1;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, (int *)&width, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, (int *)&height, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, (int *)&colorDepth, argv[2]);

    JSValue obj = JS_NewObjectClassUser(ctx, JS_CLASS_SPRITE);
    if (JS_IsException(obj)) return obj;

    SpriteData *d = (SpriteData *)malloc(sizeof(SpriteData));
    if (!d) return JS_ThrowOutOfMemory(ctx);
    d->sprite = new TFT_eSprite(&tft);
    if (!d->sprite) {
        free(d);
        return JS_ThrowOutOfMemory(ctx);
    }
    d->sprite->setColorDepth(colorDepth);
    d->sprite->createSprite(width, height, frames);

    // set opaque pointer so finalizer can free C resources
    JS_SetOpaque(ctx, obj, d);

    // register numeric sprite id (1-based) for legacy APIs and compatibility
    sprites.push_back(d->sprite);
    JS_SetPropertyStr(ctx, obj, "spritePointer", JS_NewInt32(ctx, sprites.size()));

    return obj;
#else
    return JS_NewObject(ctx);
#endif
}

JSValue native_getRotation(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewInt32(ctx, bruceConfigPins.rotation);
}

JSValue native_getBrightness(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    return JS_NewInt32(ctx, currentScreenBrightness);
}

JSValue native_setBrightness(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int brightness = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &brightness, argv[0]);
    bool save = false;
    if (argc > 1 && JS_IsObject(ctx, argv[1])) {
        JSValue v = JS_GetPropertyStr(ctx, argv[1], "save");
        if (!JS_IsUndefined(v)) save = JS_ToBool(ctx, v);
    }
    setBrightness(brightness, save);
    return JS_NewInt32(ctx, 1);
}

JSValue native_restoreBrightness(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    setBrightness(bruceConfig.bright, false);
    return JS_UNDEFINED;
}
#endif
