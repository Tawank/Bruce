Import("env")
import os
import subprocess
import hashlib

PIOENV = env.subst("$PIOENV")
MQJS_PATH = os.path.join(".pio/libdeps", PIOENV, "mquickjs")

BUILD_DIR = "mqjs_build"
GEN = os.path.join(BUILD_DIR, "mqjs_stdlib")
CHECKSUM = os.path.join(BUILD_DIR, ".mqjs_stdlib.sha256")

WATCH_FILE = "src/modules/bjs_interpreter/mqjs_stdlib.c"

SRC = [
    WATCH_FILE,
    os.path.join(MQJS_PATH, "mquickjs_build.c"),
]

CFLAGS = [
    "-Wall",
    "-O2",
    "-m32",
    "-I" + MQJS_PATH,
]

INCLUDES = [
    'audio_js',
    'globals_js'
]

def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        while True:
            chunk = f.read(8192)
            if not chunk:
                break
            h.update(chunk)
    return h.hexdigest()

def compute_checksum():
    return sha256_file(WATCH_FILE)

def needs_rebuild():
    if not os.path.exists(GEN):
        return True
    if not os.path.exists(CHECKSUM):
        return True

    with open(CHECKSUM, "r") as f:
        old = f.read().strip()

    return compute_checksum() != old

def write_checksum():
    with open(CHECKSUM, "w") as f:
        f.write(compute_checksum())

def build_generator():
    if not os.path.exists(MQJS_PATH):
        raise RuntimeError(f"mquickjs not found at {MQJS_PATH}")

    os.makedirs(BUILD_DIR, exist_ok=True)

    if needs_rebuild():
        print("Building mqjs_stdlib (host tool)")
        subprocess.check_call(["gcc", *CFLAGS, "-o", GEN, *SRC])
        write_checksum()

def generate_headers():
    print("Generating 32-bit QuickJS headers for ESP32")

    result = subprocess.run([GEN, "-m32"], capture_output=True, text=True, check=True)
    with open("src/modules/bjs_interpreter/mqjs_stdlib.h", "w") as f:
        for line in INCLUDES:
            f.write(f'#include "{line}.h"\n')
        f.write("\n")
        f.write(result.stdout)

    with open(os.path.join(MQJS_PATH, "mquickjs_atom.h"), "w") as f:
        subprocess.check_call([GEN, "-a", "-m32"], stdout=f)

build_generator()
generate_headers()
