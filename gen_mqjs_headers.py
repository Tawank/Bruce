Import("env")
import os
import subprocess
import hashlib

PIOENV = env.subst("$PIOENV")
MQJS_PATH = os.path.join(".pio/libdeps", PIOENV, "mquickjs")

BUILD_DIR = "mqjs_build"
GEN = os.path.join(BUILD_DIR, "mqjs_stdlib")
BUILD_STAMP = os.path.join(BUILD_DIR, ".mqjs_stdlib.build.stamp")
HEADERS_STAMP = os.path.join(BUILD_DIR, ".mqjs_stdlib.headers.stamp")

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
    'user_classes_js',
    'audio_js',
    'badusb_js',
    'device_js',
    'display_js',
    'dialog_js',
    'globals_js',
    'gpio_js',
    'i2c_js',
    'ir_js',
    'keyboard_js',
    'math_js',
    'notification_js',
    'serial_js',
    'storage_js',
    'subghz_js',
    'wifi_js',
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

def _project_option(name: str):
    try:
        return env.GetProjectOption(name, default="")
    except Exception:
        return ""

def _normalize_option_value(value) -> str:
    if value is None:
        return ""
    if isinstance(value, (list, tuple)):
        # Keep order; PlatformIO preserves flag order.
        return "\n".join(_normalize_option_value(v) for v in value)
    return str(value)

def compute_signature():
    # Any change here should force a rebuild.
    parts = []
    parts.append("v=2")
    parts.append(f"PIOENV={PIOENV}")
    parts.append(f"build_flags={_normalize_option_value(_project_option('build_flags'))}")
    parts.append(f"build_src_flags={_normalize_option_value(_project_option('build_src_flags'))}")
    parts.append(f"watch_sha256={sha256_file(WATCH_FILE)}")

    mqjs_build_c = os.path.join(MQJS_PATH, "mquickjs_build.c")
    if os.path.exists(mqjs_build_c):
        parts.append(f"mquickjs_build_sha256={sha256_file(mqjs_build_c)}")
    else:
        parts.append("mquickjs_build_sha256=")

    h = hashlib.sha256()
    h.update("\n".join(parts).encode("utf-8"))
    return h.hexdigest()

def needs_rebuild():
    if not os.path.exists(GEN):
        return True
    if not os.path.exists(BUILD_STAMP):
        return True

    with open(BUILD_STAMP, "r") as f:
        old = f.read().strip()

    return compute_signature() != old

def write_build_stamp():
    with open(BUILD_STAMP, "w") as f:
        f.write(compute_signature())

def needs_headers_regen():
    if not os.path.exists("src/modules/bjs_interpreter/mqjs_stdlib.h"):
        return True
    if not os.path.exists(HEADERS_STAMP):
        return True
    with open(HEADERS_STAMP, "r") as f:
        old = f.read().strip()
    return compute_signature() != old

def write_headers_stamp():
    with open(HEADERS_STAMP, "w") as f:
        f.write(compute_signature())

def build_generator():
    if not os.path.exists(MQJS_PATH):
        raise RuntimeError(f"mquickjs not found at {MQJS_PATH}")

    os.makedirs(BUILD_DIR, exist_ok=True)

    if needs_rebuild():
        print("Building mqjs_stdlib (host tool)")
        # Important: do NOT forward PlatformIO build flags to host gcc.
        # They can contain non-host-safe tokens and quoting, which breaks this build.
        subprocess.check_call(["gcc", *CFLAGS, "-o", GEN, *SRC])
        write_build_stamp()

def generate_headers():
    if not needs_headers_regen():
        return

    print("Generating 32-bit QuickJS headers for ESP32")

    result = subprocess.run([GEN, "-m32"], capture_output=True, text=True, check=True)
    with open("src/modules/bjs_interpreter/mqjs_stdlib.h", "w") as f:
        for line in INCLUDES:
            f.write(f'#include "{line}.h"\n')
        f.write("\n")
        f.write(result.stdout)

    with open(os.path.join(MQJS_PATH, "mquickjs_atom.h"), "w") as f:
        subprocess.check_call([GEN, "-a", "-m32"], stdout=f)

    write_headers_stamp()

build_generator()
generate_headers()
