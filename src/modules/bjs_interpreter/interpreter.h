#ifndef __BJS_INTERPRETER_H__
#define __BJS_INTERPRETER_H__
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "core/display.h"
#include "stdio.h"

#include <string.h>

extern char *script;
extern char *scriptDirpath;
extern char *scriptName;

// Credits to https://github.com/justinknight93/Doolittle
// This functionality is dedicated to @justinknight93 for providing such a nice example! Consider yourself a
// part of the team!

void interpreterHandler(void *pvParameters);
void run_bjs_script();
bool run_bjs_script_headless(char *code);
bool run_bjs_script_headless(FS fs, String filename);

const char *nth_strchr(const char *s, char c, int8_t n);
void js_fatal_error_handler(void *udata, const char *msg);

#endif
#endif
