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

/* Per-instance sprite storage: we attach a C pointer (opaque) to each
   JS Sprite object using JS_SetOpaque / JS_GetOpaque. The runtime will
   call `native_sprite_finalizer` when the object is collected. */
typedef struct {
    TFT_eSprite *sprite;
} SpriteData;

static inline TFT_eSprite *
get_sprite_ptr(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, int spriteArg = -1) {
    // explicit argument as object
    if (spriteArg >= 0 && argc > spriteArg && JS_IsObject(ctx, argv[spriteArg])) {
        void *opaque = JS_GetOpaque(ctx, argv[spriteArg]);
        if (opaque) return ((SpriteData *)opaque)->sprite;
        return NULL;
    }
    if (this_val && JS_IsObject(ctx, *this_val)) {
        void *opaque = JS_GetOpaque(ctx, *this_val);
        if (opaque) return ((SpriteData *)opaque)->sprite;
    }
    return NULL;
}

// helper to find sprite index: prefer explicit argument at position 'spriteArg'
static inline int
get_sprite_index(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, int spriteArg = -1) {
    if (spriteArg >= 0 && argc > spriteArg && JS_IsNumber(ctx, argv[spriteArg])) {
        int v;
        JS_ToInt32(ctx, &v, argv[spriteArg]);
        return v;
    }
    if (this_val && JS_IsObject(ctx, *this_val)) {
        JSValue v = JS_GetPropertyStr(ctx, *this_val, "spritePointer");
        if (!JS_IsUndefined(v)) {
            int tmp;
            JS_ToInt32(ctx, &tmp, v);
            return tmp;
        }
    }
    return 0;
}

#if defined(HAS_SCREEN)
static inline TFT_eSPI *get_display(JSContext *ctx) __attribute__((always_inline));
static inline TFT_eSPI *get_display(JSContext *ctx) { return static_cast<TFT_eSPI *>(&tft); }
#else
static inline SerialDisplayClass *get_display(JSContext *ctx) __attribute__((always_inline));
static inline SerialDisplayClass *get_display(JSContext *ctx) {
    return static_cast<SerialDisplayClass *>(&tft);
}
#endif

JSValue native_setTextColor(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    TFT_eSprite *sprite = get_sprite_ptr(ctx, this_val, argc, argv);
    int c = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &c, argv[0]);
    if (sprite) sprite->setTextColor(c);
    else get_display(ctx)->setTextColor(c);
    return JS_UNDEFINED;
}

JSValue native_setTextSize(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    TFT_eSprite *sprite = get_sprite_ptr(ctx, this_val, argc, argv);
    int s = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &s, argv[0]);
    if (sprite) sprite->setTextSize(s);
    else get_display(ctx)->setTextSize(s);
    return JS_UNDEFINED;
}

JSValue native_setTextAlign(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    TFT_eSprite *sprite = get_sprite_ptr(ctx, this_val, argc, argv);
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
    if (sprite) sprite->setTextDatum(align + baseline * 3);
    else get_display(ctx)->setTextDatum(align + baseline * 3);
    return JS_UNDEFINED;
}

JSValue native_drawRect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    TFT_eSprite *sprite = get_sprite_ptr(ctx, this_val, argc, argv);
    int a = 0, b = 0, c = 0, d = 0, e = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &d, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &e, argv[4]);
    if (sprite) sprite->drawRect(a, b, c, d, e);
    else get_display(ctx)->drawRect(a, b, c, d, e);
    return JS_UNDEFINED;
}

JSValue native_drawFillRect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    TFT_eSprite *sprite = get_sprite_ptr(ctx, this_val, argc, argv);
    int a = 0, b = 0, c = 0, d = 0, e = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &d, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &e, argv[4]);
    if (sprite) sprite->fillRect(a, b, c, d, e);
    else get_display(ctx)->fillRect(a, b, c, d, e);
    return JS_UNDEFINED;
}

JSValue native_drawFillRectGradient(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(HAS_SCREEN)
    TFT_eSprite *sprite = get_sprite_ptr(ctx, this_val, argc, argv);
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
        if (sprite) sprite->fillRectHGradient(a, b, c, d, e, f);
        else get_display(ctx)->fillRectHGradient(a, b, c, d, e, f);
    } else {
        if (sprite) sprite->fillRectVGradient(a, b, c, d, e, f);
        else get_display(ctx)->fillRectVGradient(a, b, c, d, e, f);
    }
#else
    int sprite = get_sprite_index(ctx, this_val, argc, argv);
    int a = 0, b = 0, c = 0, d = 0, e = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &d, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &e, argv[4]);
    if (sprite) sprite->fillRect(a, b, c, d, e);
    else get_display(ctx)->fillRect(a, b, c, d, e);
#endif
    return JS_UNDEFINED;
}

JSValue native_drawRoundRect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    TFT_eSprite *sprite = get_sprite_ptr(ctx, this_val, argc, argv);
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &d, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &e, argv[4]);
    if (argc > 5 && JS_IsNumber(ctx, argv[5])) JS_ToInt32(ctx, &f, argv[5]);
    if (sprite) sprite->drawRoundRect(a, b, c, d, e, f);
    else get_display(ctx)->drawRoundRect(a, b, c, d, e, f);
    return JS_UNDEFINED;
}

JSValue native_drawFillRoundRect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    TFT_eSprite *sprite = get_sprite_ptr(ctx, this_val, argc, argv);
    int a = 0, b = 0, c = 0, d = 0, e = 0, f = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &d, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &e, argv[4]);
    if (argc > 5 && JS_IsNumber(ctx, argv[5])) JS_ToInt32(ctx, &f, argv[5]);
    if (sprite) sprite->fillRoundRect(a, b, c, d, e, f);
    else get_display(ctx)->fillRoundRect(a, b, c, d, e, f);
    return JS_UNDEFINED;
}

JSValue native_drawCircle(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    TFT_eSprite *sprite = get_sprite_ptr(ctx, this_val, argc, argv);
    int a = 0, b = 0, c = 0, d = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &d, argv[3]);
    if (sprite) sprite->drawCircle(a, b, c, d);
    else get_display(ctx)->drawCircle(a, b, c, d);
    return JS_UNDEFINED;
}

JSValue native_drawFillCircle(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    TFT_eSprite *sprite = get_sprite_ptr(ctx, this_val, argc, argv);
    int a = 0, b = 0, c = 0, d = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &d, argv[3]);
    if (sprite) sprite->fillCircle(a, b, c, d);
    else get_display(ctx)->fillCircle(a, b, c, d);
    return JS_UNDEFINED;
}

JSValue native_drawLine(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    TFT_eSprite *sprite = get_sprite_ptr(ctx, this_val, argc, argv);
    int a = 0, b = 0, c = 0, d = 0, e = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &d, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &e, argv[4]);
    if (sprite) sprite->drawLine(a, b, c, d, e);
    else get_display(ctx)->drawLine(a, b, c, d, e);
    return JS_UNDEFINED;
}

JSValue native_drawPixel(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    TFT_eSprite *sprite = get_sprite_ptr(ctx, this_val, argc, argv);
    int a = 0, b = 0, c = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &a, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &b, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &c, argv[2]);
    if (sprite) sprite->drawPixel(a, b, c);
    else get_display(ctx)->drawPixel(a, b, c);
    return JS_UNDEFINED;
}

JSValue native_drawXBitmap(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // Accept strings for now or throw if not a string (ArrayBuffer support to add later)
    int bitmapWidth = 0, bitmapHeight = 0;
    if (argc > 3 && JS_IsNumber(ctx, argv[3])) JS_ToInt32(ctx, &bitmapWidth, argv[3]);
    if (argc > 4 && JS_IsNumber(ctx, argv[4])) JS_ToInt32(ctx, &bitmapHeight, argv[4]);
    size_t len = 0;
    const char *data = NULL;
    if (argc > 2 && JS_IsString(ctx, argv[2])) {
        JSCStringBuf sb;
        data = JS_ToCStringLen(ctx, &len, argv[2], &sb);
    } else {
        return JS_ThrowTypeError(ctx, "%s: Expected string/ArrayBuffer for bitmap data", "drawXBitmap");
    }
    size_t expectedSize = ((bitmapWidth + 7) / 8) * bitmapHeight;
    if (len != expectedSize) return JS_ThrowTypeError(ctx, "Bitmap size mismatch");
    TFT_eSprite *sprite = get_sprite_ptr(ctx, this_val, argc, argv);
    int x = 0, y = 0, fg = 0, bg = -1;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    if (argc > 5 && JS_IsNumber(ctx, argv[5])) JS_ToInt32(ctx, &fg, argv[5]);
    if (argc > 6 && JS_IsNumber(ctx, argv[6])) JS_ToInt32(ctx, &bg, argv[6]);
    if (bg >= 0) {
        if (sprite) sprite->drawXBitmap(x, y, (uint8_t *)data, bitmapWidth, bitmapHeight, fg, bg);
        else get_display(ctx)->drawXBitmap(x, y, (uint8_t *)data, bitmapWidth, bitmapHeight, fg, bg);
    } else {
        if (sprite) sprite->drawXBitmap(x, y, (uint8_t *)data, bitmapWidth, bitmapHeight, fg);
        else get_display(ctx)->drawXBitmap(x, y, (uint8_t *)data, bitmapWidth, bitmapHeight, fg);
    }
    return JS_UNDEFINED;
}

JSValue native_drawString(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    int sprite = get_sprite_index(ctx, this_val, argc, argv);
    if (argc > 0 && JS_IsString(ctx, argv[0])) {
        JSCStringBuf sb;
        const char *s = JS_ToCString(ctx, argv[0], &sb);
        int x = 0, y = 0;
        if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &x, argv[1]);
        if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, &y, argv[2]);
        get_display(ctx)->drawString(s, x, y);
    }
    return JS_UNDEFINED;
}

JSValue native_setCursor(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    TFT_eSprite *sprite = get_sprite_ptr(ctx, this_val, argc, argv);
    int x = 0, y = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &x, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, &y, argv[1]);
    if (sprite) sprite->setCursor(x, y);
    else get_display(ctx)->setCursor(x, y);
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
    TFT_eSprite *sprite = get_sprite_ptr(ctx, this_val, argc, argv);
    int c = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &c, argv[0]);
    if (!sprite) {
        tft.fillScreen(c);
    } else {
#if defined(HAS_SCREEN)
        sprite->fillSprite(c);
#endif
    }
    return JS_UNDEFINED;
}

JSValue native_width(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    TFT_eSprite *sprite = get_sprite_ptr(ctx, this_val, argc, argv);
    int width = sprite ? sprite->width() : get_display(ctx)->width();
    return JS_NewInt32(ctx, width);
}

JSValue native_height(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    TFT_eSprite *sprite = get_sprite_ptr(ctx, this_val, argc, argv);
    int height = sprite ? sprite->height() : get_display(ctx)->height();
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

void clearDisplayModuleData() { clearGifsVector(); }

/* Finalizer called by mquickjs when a Sprite JS object is freed. */
void native_sprite_finalizer(JSContext *ctx, void *opaque) {
    SpriteData *d = (SpriteData *)opaque;
    if (!d) return;
#if defined(HAS_SCREEN)
    if (d->sprite) {
        delete d->sprite;
        d->sprite = NULL;
    }
#endif
    free(d);
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

JSValue native_deleteSprite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    JSValue obj = JS_UNDEFINED;
    if (argc > 0 && JS_IsObject(ctx, argv[0])) obj = argv[0];
    else if (this_val && JS_IsObject(ctx, *this_val)) obj = *this_val;
    if (JS_IsUndefined(obj)) return JS_NewInt32(ctx, 0);

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
    TFT_eSprite *sprite = get_sprite_ptr(ctx, this_val, argc, argv);
    if (sprite) sprite->pushSprite(0, 0);
#endif
#endif
    return JS_UNDEFINED;
}

JSValue native_createSprite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(HAS_SCREEN)
    JSGCRef obj_ref;
    JSValue *objp = JS_PushGCRef(ctx, &obj_ref);
    *objp = JS_NewObjectClassUser(ctx, JS_CLASS_SPRITE);

    SpriteData *d = (SpriteData *)malloc(sizeof(SpriteData));
    if (!d) {
        JS_PopGCRef(ctx, &obj_ref);
        return JS_ThrowOutOfMemory(ctx);
    }
    d->sprite = nullptr;

    int16_t width = tft.width();
    int16_t height = tft.height();
    uint8_t colorDepth = 16;
    uint8_t frames = 1;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, (int *)&width, argv[0]);
    if (argc > 1 && JS_IsNumber(ctx, argv[1])) JS_ToInt32(ctx, (int *)&height, argv[1]);
    if (argc > 2 && JS_IsNumber(ctx, argv[2])) JS_ToInt32(ctx, (int *)&colorDepth, argv[2]);

    // allocate sprite using new/delete so destructor is correct
    d->sprite = new TFT_eSprite(&tft);
    if (!d->sprite) {
        free(d);
        JS_PopGCRef(ctx, &obj_ref);
        return JS_ThrowOutOfMemory(ctx);
    }
    d->sprite->setColorDepth(colorDepth);
    d->sprite->createSprite(width, height, frames);

    JS_SetOpaque(ctx, *objp, d);
    JS_PopGCRef(ctx, &obj_ref);
    return *objp;
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
