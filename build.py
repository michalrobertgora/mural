import gzip
import os
import shutil
import subprocess
Import("env")

# Pre-build step: build the TS worker and React UI into data/www, then gzip them.
# Uses only portable Python file operations so it runs on Windows as well as Linux/macOS.

PROJECT_DIR = env.subst("$PROJECT_DIR")
WWW_DIR = os.path.join(PROJECT_DIR, "data", "www")
WORKER_DIR = os.path.join(PROJECT_DIR, "worker")
UI_DIR = os.path.join(PROJECT_DIR, "ui")


def npm(cwd, *args):
    """Run an npm command in cwd, failing the build if it fails.

    Output is captured and re-printed as plain ASCII: vite/webpack emit Unicode symbols (e.g. a
    check mark) that PlatformIO's output echo can't encode on Windows (cp1252), which kills its
    reader thread and hangs the build."""
    command = ["npm"] + list(args)
    print("[%s] %s" % (os.path.basename(cwd), " ".join(command)))
    proc = subprocess.run(command, cwd=cwd, shell=(os.name == "nt"),
                          stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    text = proc.stdout.decode("utf-8", errors="replace")
    print(text.encode("ascii", errors="replace").decode("ascii"))
    if proc.returncode != 0:
        print("npm %s failed in %s (exit %d)" % (" ".join(args), cwd, proc.returncode))
        env.Exit(1)


def ensure_node_modules(cwd, *extra):
    if not os.path.isdir(os.path.join(cwd, "node_modules")):
        has_lock = os.path.exists(os.path.join(cwd, "package-lock.json"))
        npm(cwd, "ci" if has_lock else "install", *extra)


print("Transpiling TS worker code")
shutil.rmtree(WWW_DIR, ignore_errors=True)
# canvas (native, node-gyp) is only needed by the dev-only tester.ts, never by the bundle;
# skipping install scripts avoids needing a C++ toolchain + cairo on Windows.
ensure_node_modules(WORKER_DIR, "--ignore-scripts")
npm(WORKER_DIR, "run", "build")
worker_out = os.path.join(WWW_DIR, "worker")
os.makedirs(worker_out, exist_ok=True)
packed = os.path.join(WORKER_DIR, "dist_packed")
shutil.copyfile(os.path.join(packed, "main.js"), os.path.join(worker_out, "worker.js"))
shutil.copyfile(os.path.join(packed, "oneline.js"), os.path.join(worker_out, "oneline.js"))
shutil.copyfile(os.path.join(packed, "oneline.wasm"), os.path.join(worker_out, "oneline.wasm"))

print("Building React UI")
ensure_node_modules(UI_DIR)
npm(UI_DIR, "run", "build")

print("Gzip compressing static assets")
for dirpath, _, filenames in os.walk(WWW_DIR):
    for filename in filenames:
        src = os.path.join(dirpath, filename)
        dst = src + ".gz"
        with open(src, "rb") as f_in, gzip.open(dst, "wb", compresslevel=9) as f_out:
            shutil.copyfileobj(f_in, f_out)
        os.remove(src)
