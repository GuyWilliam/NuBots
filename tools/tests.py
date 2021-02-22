#!/usr/bin/env python3

import multiprocessing
import os
import subprocess
import time

import b
from dockerise import run_on_docker

# Directory to create in project root for storing output of tests
TESTS_OUTPUT_DIR = "tests_output"


@run_on_docker
def register(command):

    command.help = "Run and list tests"
    subparsers = command.add_subparsers(help="sub-command help", dest="sub_command")

    command_run = subparsers.add_parser("run")
    command_list = subparsers.add_parser("list")

    command_run.add_argument(
        "-VV",
        "--extra-verbose",
        action="append_const",
        dest="given_ctest_args",
        const="-VV",
        help="Enable more verbose output from tests",
    )

    command_run.add_argument(
        "-V",
        "--verbose",
        action="append_const",
        dest="given_ctest_args",
        const="-V",
        help="Enable verbose output from tests",
    )

    command_run.add_argument(
        "-Q",
        "--quiet",
        action="append_const",
        dest="given_ctest_args",
        const="-Q",
        help="Make ctest not print to stdout",
    )

    command_run.add_argument(
        "-j",
        "--parallel",
        action="store",
        dest="num_jobs",
        default=multiprocessing.cpu_count(),
        help="Run the tests in parallel using the given number of jobs.",
    )

    command_run.add_argument(
        "--debug",
        action="append_const",
        dest="given_ctest_args",
        const="--debug",
        help="Displaying more verbose internals of CTest",
    )

    command_run.add_argument(
        "test", nargs="?", help="Name of test to run (will run all tests cointaining given string)"
    )


@run_on_docker
def run(sub_command, num_jobs=0, test=None, given_ctest_args=[], **kwargs):

    # Change into the build directory
    os.chdir(os.path.join(b.project_dir, "..", "build"))

    if sub_command == "list":
        exit(subprocess.run(["/usr/bin/ctest", "-N"]).returncode)

    if sub_command == "run":
        tests_dir = os.path.join(b.project_dir, TESTS_OUTPUT_DIR)

        # If tests dir not at /home/nubots/Nubots/tests_output, create it
        if not os.path.exists(tests_dir):
            os.makedirs(tests_dir)

        # Windows friendly (container time, not host)
        filename = time.strftime("%Y-%m-%d-%H-%M-%S") + ".log"

        # Default ctest args
        ctest_args = [
            "--force-new-ctest-process",
            "--output-on-failure",
        ]

        # Add given args to default args
        if given_ctest_args:
            ctest_args.extend(given_ctest_args)

        # If a test was given to run
        if test:
            filename = test + "-" + filename
            logPath = os.path.join(tests_dir, filename)
            exit(
                subprocess.run(
                    ["/usr/bin/ctest", "--output-log", logPath, "--parallel", str(num_jobs), "-R", test, *ctest_args,]
                ).returncode
            )

        # If no test was given to run, run all
        filename = "all-" + filename
        logPath = os.path.join(tests_dir, filename)
        exit(
            subprocess.run(
                ["/usr/bin/ctest", "--output-log", logPath, "--parallel", str(num_jobs), *ctest_args]
            ).returncode
        )

    else:
        # Probably a better way to call help when no sub commands are given?
        os.chdir(b.project_dir)
        tool_name = os.path.basename(__file__)[:-3]
        exit(subprocess.run(["./b", tool_name, "--help"]).returncode)
