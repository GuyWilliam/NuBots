#!/usr/bin/env python3

import argparse
import os

import b
from utility.dockerise import run_on_docker
from utility.roles import all_role_names, role_folders, dir_role_names
from utility.shell import WrapPty

ROLE_GROUPS = {
    "all" : all_role_names(),
    "cool" : ["robocup", "test/camera"],
    "nice" : ["scripttuner", "natnet"],
}

for r in role_folders():
    ROLE_GROUPS[r] = dir_role_names(r)

class ExtendConstAction(argparse._AppendConstAction):
    def __call__(self, parser, namespace, values, option_string=None):
        items = getattr(namespace, self.dest, None)
        items = argparse._copy_items(items)
        items.extend(self.const)
        setattr(namespace, self.dest, items)

@run_on_docker
def register(command):
    # Install help
    command.help = "Configure the project in a docker container"

    command.add_argument(
        "-i",
        "--interactive",
        dest="interactive",
        action="store_true",
        default=False,
        help="perform an interactive configuration using ccmake",
    )
    command.add_argument(
        "--unset_roles",
        dest="unset_roles",
        action="extend",
        default=[],
        choices=all_role_names(),
        nargs="*",
        help="Unset roles via command line"
    )
    for group in ROLE_GROUPS.keys():
        command.add_argument(
            f"--unset_{group}_roles",
            dest="unset_roles",
            action=ExtendConstAction,
            const=ROLE_GROUPS[group],
        )
    command.add_argument(
        "--set_roles",
        dest="set_roles",
        action="extend",
        default=[],
        choices=all_role_names(),
        nargs="+",
        help="Set roles via command line"
    )
    for group in ROLE_GROUPS.keys():
        command.add_argument(
            f"--set_{group}_roles",
            dest="set_roles",
            action=ExtendConstAction,
            const=ROLE_GROUPS[group],
        )
    command.add_argument("args", nargs="...", help="the arguments to pass through to cmake")


@run_on_docker
def run(interactive, set_roles, unset_roles, args, **kwargs):
    pty = WrapPty()

    # If interactive then run ccmake else just run cmake
    os.chdir(os.path.join(b.project_dir, "..", "build"))

    # To pass arguments to the cmake command you put them after "--"
    # but "--"  isn't a valid argument for cmake, so we remove it here
    if "--" in args:
        args.remove("--")

    for role in unset_roles:
        role = role.replace("/","-")
        args.append(f"-DROLE_{role}:BOOL=OFF")
    for role in set_roles:
        role = role.replace("/","-")
        args.append(f"-DROLE_{role}:BOOL=ON")

    if interactive:
        exit(
            pty.spawn(["ccmake", "-GNinja", "-DCMAKE_TOOLCHAIN_FILE=/usr/local/toolchain.cmake", *args, b.project_dir])
        )
    else:
        exit(pty.spawn(["cmake", "-GNinja", "-DCMAKE_TOOLCHAIN_FILE=/usr/local/toolchain.cmake", *args, b.project_dir]))
