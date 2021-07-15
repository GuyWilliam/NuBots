#!/usr/bin/env python3

import os
import re

from termcolor import cprint

import b
from utility.dockerise import run_on_docker


@run_on_docker
def register(command):
    # Install help
    command.help = "Creates a list of unused or commented out modules"

    command.add_argument(
        "-g",
        "--generate-role",
        dest="generate_role",
        action="store_true",
        help="Generate a role which includes the unused modules",
    )


@run_on_docker
def run(generate_role, **kwargs):

    # If we ever generate a role with the unused modules, this is what it'll be called
    UNUSED_ROLE_NAME = "__unused__.role"

    source_dir = b.cmake_cache[b.cmake_cache["CMAKE_PROJECT_NAME"] + "_SOURCE_DIR"]
    roles_path = os.path.join(source_dir, b.cmake_cache["NUCLEAR_ROLES_DIR"])
    modules_path = os.path.join(source_dir, b.cmake_cache["NUCLEAR_MODULE_DIR"])

    # Modules that exist in the system
    existing_modules = set()

    # Modules that are used in role files
    used_modules = set()

    # Modules that are not used by any role
    unused_modules = set()

    # Find all CMakeLists.txt in NUCLEAR_MODULE_DIR that contain a nuclear_module() call
    for folder, _, files in os.walk(modules_path):
        for module in files:
            if module == "CMakeLists.txt":
                module_path = os.path.join(folder, module)
                with open(module_path, "r") as f:
                    for line in f:
                        if "nuclear_module" in line.lower():
                            # Find nuclear_module call and make sure it is not commented out
                            if "#" not in line or line.find("#") > line.lower().find("nuclear_module"):
                                # Remove modules_path from the start of the path and
                                # CMakeLists.txt from the end of the path
                                # So module/motion/HeadController/CMakeLists.txt becomes motion/HeadController
                                path = module_path.replace(modules_path, "")
                                path = os.path.join(*(path.split(os.path.sep)[:-1]))

                                # Replace path separators with ::
                                path = path.replace(os.path.sep, "::")
                                existing_modules.add(path)
                                break

    # Find all of the used modules
    for role in os.listdir(roles_path):
        if role.endswith(".role") and role != UNUSED_ROLE_NAME:
            with open(os.path.join(roles_path, role), "r") as f:
                for line in f:
                    if "#" in line:
                        line = line[: line.find("#")]
                    line = line.strip()
                    reg = re.findall(r"(\w+::(?:\w+(::)?)*)", line)
                    for modules in reg:
                        for module in modules:
                            if module != "" and module != "::":
                                used_modules.add(module)

    # Find out which modules are unused
    unused_modules = existing_modules.difference(used_modules)

    cprint("Existing Modules", attrs=["bold"])
    print("\n".join(existing_modules))
    print("\n")

    cprint("Used Modules", "green", attrs=["bold"])
    cprint("\n".join(used_modules), "green")
    print("\n")

    cprint("Unused Modules", "red", attrs=["bold"])
    cprint("\n".join(unused_modules), "red", attrs=["bold"])
    print("\n")

    # If we want to make a role with the unused modules to check if they compile, we do that now
    if generate_role:
        with open(os.path.join(roles_path, UNUSED_ROLE_NAME), "w") as unused_role:
            unused_role.write("nuclear_role(\n  ")
            unused_role.write("\n  ".join(unused_modules))
            unused_role.write("\n)")
        # Warn the user that the new role won't build unless they reconfigure
        cprint("You must run ./b configure after generating a new role", "red", attrs=["bold"])
