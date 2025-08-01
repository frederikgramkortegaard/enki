#!/usr/bin/env python3
from collections import namedtuple
import shutil
from subprocess import run, PIPE
import argparse
import re
from ast import literal_eval
from dataclasses import dataclass
from enum import Enum
from os import system, makedirs
import os
from pathlib import Path
from sys import argv
from typing import Union, Optional, Tuple
import multiprocessing
import sys
import shlex
import json
import textwrap

class Result(Enum):
    EXIT_WITH_CODE = 1
    EXIT_WITH_OUTPUT = 2
    COMPILE_FAIL = 3
    COMPILE_SUCCESS = 4
    TYPECHECK = 5
    SKIP_SILENTLY = 6
    SKIP_REPORT = 7
    RUNTIME_FAIL = 8

@dataclass(frozen=True)
class Expected:
    type: Result
    value: Union[int, str, None]


def get_expected(filename) -> Optional[Expected]:
    with open(filename, encoding="utf8", errors='ignore') as file:
        for line in file:
            if not line.startswith("///"):
                break

            line = line[3:].strip()

            # Commands with no arguments
            if line == "skip":
                return Expected(Result.SKIP_SILENTLY, None)
            if line == "compile":
                return Expected(Result.COMPILE_SUCCESS, None)
            if line == "":
                continue

            if ":" not in line:
                print(f'[-] Invalid parameters in {filename}: "{line}"')
                break

            # Commands with arguments
            name, value = map(str.strip, line.split(":", 1))

            if name == "exit":
                return Expected(Result.EXIT_WITH_CODE, int(value))
            if name == "out":
                return Expected(Result.EXIT_WITH_OUTPUT, value)
            if name == "fail":
                return Expected(Result.COMPILE_FAIL, value)
            if name == "runfail":
                return Expected(Result.RUNTIME_FAIL, value)

            print(f'[-] Invalid parameter in {filename}: {line}')
            break

    # TODO: Remove these once we have added the test modes into the files as opposed
    #       to just using the file name prefixes.
    base = Path(filename).stem.lower()
    if base.endswith("_success"):
        return Expected(Result.EXIT_WITH_CODE, 0)
    if base.endswith("_fail"):
        return Expected(Result.COMPILE_FAIL, "")
    if base.endswith("_error"):
        return Expected(Result.COMPILE_FAIL, "")
    if base.startswith("syntax_error_"):
        return Expected(Result.COMPILE_FAIL, "")
    if base.endswith("_typecheck"):
        return Expected(Result.TYPECHECK, None)
    if base.endswith("_typecheck"):
        return Expected(Result.TYPECHECK, None)

    return Expected(Result.EXIT_WITH_CODE, 0)

def compile_file(compiler, src, output, expected):
    compiler = os.path.abspath(compiler)

    extra_flags = ""
    if expected.type == Result.TYPECHECK:
        extra_flags = "-t"
    cmd = f"{compiler} compile -a -o {output} {extra_flags} {src}"
    return run(
        cmd,
        stdout=PIPE,
        stderr=PIPE,
        shell=True,
    )


def handle_test(compiler: str, num: int, path: Path, expected: Expected, debug: bool) -> Tuple[bool, str, Path]:
    exec_name = f'./build/tests/{path.stem}-{num}'
    if debug:
        print(f"[{num}] {path} || {exec_name}", flush=True)

    process = compile_file(compiler, path, exec_name, expected)
    if expected.type == Result.COMPILE_FAIL:
        if process.returncode == 0:
            return False, "Expected compilation failure, but succeeded", path

        error = process.stdout.decode("utf-8").strip()
        error += process.stderr.decode("utf-8").strip()
        expected_error = expected.value

        if expected_error in error:
            return True, "(Success)", path
        else:
            try:
                remaining = error.split("Error: ")[1]
            except IndexError:
                remaining = error
            return False, f"Did not find expected error message\n  expected: {expected_error}\n       got: '{remaining}'", path

    elif process.returncode != 0:
        stdout = textwrap.indent(process.stdout.decode("utf-8"), " "*10).strip()
        stderr = textwrap.indent(process.stderr.decode("utf-8"), " "*10).strip()
        return False, f"Compilation failed:\n  code: {process.returncode}\n  stdout: {stdout}\n  stderr: {stderr}", path

    elif expected.type in (Result.COMPILE_SUCCESS, Result.TYPECHECK):
        return True, "(Success)", path

    try:
        process = run([exec_name], stdout=PIPE, stderr=PIPE)
    except FileNotFoundError:
        return False, "Executable not found", path

    if process.returncode != 0 and expected.type not in [Result.EXIT_WITH_CODE, Result.RUNTIME_FAIL]:
        return False, f"Expected exit code 0, but got {process.returncode}", path

    if expected.type == Result.EXIT_WITH_CODE:
        if process.returncode != expected.value:
            return False, f"Expected exit code {expected.value}, but got {process.returncode}", path

    if expected.type == Result.EXIT_WITH_OUTPUT:
        stdout_output = process.stdout.decode('utf-8').strip()
        stderr_output = process.stderr.decode('utf-8').strip()
        output = (stdout_output + '\n' + stderr_output).strip()
        expected_out = literal_eval(expected.value).strip()
        if output != expected_out:
            return False, f'Incorrect output produced\n  expected: {repr(expected_out)}\n       got: {repr(output)}', path

    if expected.type == Result.RUNTIME_FAIL:
        output = process.stdout.decode('utf-8').strip()
        output_err = process.stderr.decode('utf-8').strip()
        expected_out = expected.value
        all_out = output + output_err

        if expected_out not in all_out:
            return False, f'Expected runtime error not found\n  expected: {repr(expected_out)}\n  got: {repr(all_out)}', path

    return True, "(Success)", path

def pool_helper(args):
    return handle_test(*args)

def output_test_results(result, stats, debug):
    passed, message, path = result
    if sys.stdout.isatty():
        print(f" \33[2K[\033[92m{stats.passed:3d}\033[0m", end="")
        print(f"/\033[91m{stats.failed:3d}\033[0m]", end="")
        print(f" Running tests, finished {stats.passed+stats.failed} / {stats.total}\r", end="", flush=True)
    if passed:
        stats.passed += 1
    else:
        stats.failed += 1
        if sys.stdout.isatty():
            print(f"\33[2K\033[91m[-] Failed {path}\033[0m")
            print(f"  - {message}", flush=True)
        else:
            print(f"[-] Failed {path}")
    if debug and not passed:
        exit(1)


def main():
    parser = argparse.ArgumentParser(description="Runs enki test suite")
    parser.add_argument(
        "-c",
        "--compiler",
        default=None,
        help="Path to the compiler executable (default: build)"
    )
    parser.add_argument(
        "-d",
        "--debug",
        action="store_true",
        help="Run single threaded, and print all test names as we go"
    )
    parser.add_argument(
        "files",
        nargs="?",
        default=["tests"],
        help="Files / folders to run"
    )
    args = parser.parse_args()
    arg_files = args.files if isinstance(args.files, list) else [args.files]
    test_paths = [Path(pth) for pth in arg_files]

    if args.compiler is None:
        system("make -j")
        if not Path("./enki").is_file():
            print("Compiler not found, please run from root or specify the compiler path with -c")
            exit(1)
        args.compiler = "./enki"

    tests_to_run = []
    for path in test_paths:
        files = []

        if path.is_dir():
            for path_ in path.glob('**/*.enki'):
                if path_.is_file():
                    files.append(path_)
        else:
            files.append(path)

        for file in files:
            expected = get_expected(file)
            if expected.type == Result.SKIP_SILENTLY:
                continue
            if expected.type == Result.SKIP_REPORT:
                print(f'[-] Skipping {file}')
                continue
            tests_to_run.append((file, expected))

    tests_to_run.sort()

    # Dummy empty class to store stats, and pass mutable object to pool_helper
    class Stats(): pass
    stats = Stats()
    stats.passed = 0
    stats.failed = 0
    stats.total = len(tests_to_run)

    arguments = [
        (args.compiler, num, test_path, expected, args.debug)
        for num, (test_path, expected) in enumerate(tests_to_run)
    ]

    # Clear out existing test directories
    shutil.rmtree("build/tests", ignore_errors=True)
    makedirs("build/tests", exist_ok=True)

    if args.debug:
        for inp in arguments:
            result = pool_helper(inp)
            output_test_results(result, stats, True)

    else:
        with multiprocessing.Pool(multiprocessing.cpu_count()) as pool:
            for result in pool.imap_unordered(pool_helper, arguments):
                output_test_results(result, stats, False)

    if sys.stdout.isatty():
        print("\33[2K")
        print(f"Tests passed: \033[92m{stats.passed}\033[0m")
        print(f"Tests failed: \033[91m{stats.failed}\033[0m")

    if stats.failed > 0:
        exit(1)


if __name__ == "__main__":
    main()
