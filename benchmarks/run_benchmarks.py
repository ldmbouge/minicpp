#!/usr/bin/env python3
"""Minimal RCPSP benchmark runner (parallel).

Usage:
    python run.py --args "<solver_args>" -i <instances.py> [-j N]

Example:
    python run.py --args "-h SML -a TT" -i rcpsp_instances.py -j 4
"""

import argparse
import importlib.util
import subprocess
import threading
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime
from pathlib import Path

# ---------------------------------------------------------------------------
# CONFIG
# ---------------------------------------------------------------------------
EXES = ["./rcpsp_cli"]          # solver executables to run
INSTANCE_DIR = "../data"        # paths from the instances file are relative to this
LOG_DIR = "./logs"
TIMEOUT = 300                   # solver timeout (s); passed as -t and used in name
KILL_GRACE = 30                 # `timeout` kills this many seconds past TIMEOUT
# ---------------------------------------------------------------------------


def load_instances(py_file):
    spec = importlib.util.spec_from_file_location("instances", py_file)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    return mod.ALL


def arg_tag(args):
    # "-h SML -a TT" -> "hSML_aTT"  (strip dashes, join each flag with its value)
    toks = args.split()
    parts, i = [], 0
    while i < len(toks):
        flag = toks[i].lstrip("-")
        if i + 1 < len(toks) and not toks[i + 1].startswith("-"):
            parts.append(f"{flag}{toks[i + 1]}")
            i += 2
        else:
            parts.append(flag)
            i += 1
    return "_".join(parts)


def run_one(exe, solver_args, instance, instance_dir):
    """Run a single solve, returning (command_str, captured_output)."""
    solver = [exe, *solver_args.split(),
              "-t", str(TIMEOUT),
              "-i", str(instance_dir / instance)]
    cmd = ["/usr/bin/time", "-v",
           "timeout", str(TIMEOUT + KILL_GRACE),
           *solver]
    proc = subprocess.run(cmd, stdout=subprocess.PIPE,
                          stderr=subprocess.STDOUT, text=True)
    return " ".join(cmd), proc.stdout


def run_exe(exe, solver_args, instances, instance_dir, log_dir, jobs):
    tag = arg_tag(solver_args)
    base = f"{Path(exe).name}_t{TIMEOUT}_{tag}_{datetime.now():%d%m%Y}"
    log_path = log_dir / f"{base}.log"
    n = 1
    while log_path.exists():
        log_path = log_dir / f"{base}_{n}.log"
        n += 1

    total = len(instances)
    print(f"{exe}: {total} instances, {jobs} workers -> {log_path}")

    done = 0
    lock = threading.Lock()

    with open(log_path, "w") as log:
        log.write(f"exe: {exe}\n")
        log.write(f"args: {solver_args} -t {TIMEOUT}\n")
        log.write(f"instances: {total}\n")
        log.write("----\n")
        log.flush()

        def task(inst):
            nonlocal done
            cmd_str, output = run_one(exe, solver_args, inst, instance_dir)
            with lock:                      # one writer at a time
                log.write(f"\ncommand: {cmd_str}\n")
                log.write(output)
                log.flush()
                done += 1
                print(f"[{done}/{total}] done: {inst}", flush=True)

        with ThreadPoolExecutor(max_workers=jobs) as pool:
            list(pool.map(task, instances))

    return log_path


def main():
    ap = argparse.ArgumentParser(description="Minimal benchmark runner (parallel).")
    ap.add_argument("--args", required=True, default="",
                    help='Solver argument string (no -t), e.g. "-h SML -a TT". '
                         'If it starts with a dash, use --args="-h SML" (with =).')
    ap.add_argument("-i", "--instances", required=True,
                    help="Python file exposing ALL (list of instance paths).")
    ap.add_argument("-j", "--jobs", type=int, default=1,
                    help="Number of parallel solver runs (default: 1).")
    opts = ap.parse_args()

    instances = load_instances(opts.instances)
    instance_dir = Path(INSTANCE_DIR)
    log_dir = Path(LOG_DIR)
    log_dir.mkdir(parents=True, exist_ok=True)

    for exe in EXES:
        run_exe(exe, opts.args, instances, instance_dir, log_dir, opts.jobs)


if __name__ == "__main__":
    main()