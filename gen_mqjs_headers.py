Import("env")
import os
import subprocess

PIOENV = env.subst("$PIOENV")
MQJS_PATH = os.path.join(".pio/libdeps", PIOENV, "mquickjs");

BUILD_DIR = "mqjs_build"
GEN = os.path.join(BUILD_DIR, "mqjs_stdlib")

SRC = [
    "src/modules/bjs_interpreter/mqjs_stdlib.c",
    os.path.join(MQJS_PATH, "mquickjs_build.c"),
]

CFLAGS = [
    "-Wall",
    "-O2",
    "-m32",
    "-Ilib/mquickjs"
]

def build_generator():
    if not os.path.exists(BUILD_DIR):
        os.mkdir(BUILD_DIR)

    # rebuild if generator missing
    if not os.path.exists(GEN):
        print("Building mqjs_stdlib (host tool)")
        subprocess.check_call(
            ["gcc", *CFLAGS, "-o", GEN, *SRC]
        )

def generate_headers():
    print("Generating 32-bit QuickJS headers for ESP32")

    with open("src/modules/bjs_interpreter/mqjs_stdlib.h", "w") as f:
        subprocess.check_call([GEN, "-m32"], stdout=f)

    with open(os.path.join(MQJS_PATH, "mquickjs_atom.h"), "w") as f:
        subprocess.check_call([GEN, "-a", "-m32"], stdout=f)

build_generator()
generate_headers()
