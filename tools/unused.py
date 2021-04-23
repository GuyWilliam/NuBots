#!/usr/bin/env python3

import os
import re

import b
from utility.dockerise import run_on_docker


@run_on_docker
def register(command):
    # Install help
    command.help = "Creates a list of unused or commented out modules"


@run_on_docker
def run(**kwargs):

    source_dir = b.cmake_cache[b.cmake_cache["CMAKE_PROJECT_NAME"] + "_SOURCE_DIR"]
    roles_path = os.path.join(source_dir, "roles")
    modules_path = os.path.join(source_dir, b.cmake_cache["NUCLEAR_MODULE_DIR"])

    # Modules that exist in the system
    existing_modules = set()

    # Modules that are used in role files
    used_modules = set()

    # Modules that are not used by any role
    missing_modules = set()

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
        if role.endswith(".role"):
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

    # Find out which modules are missing
    missing_modules = existing_modules.difference(used_modules)

    print("Existing")
    print("\n".join(existing_modules))
    print("\n")

    print("Used")
    print("\n".join(used_modules))
    print("\n")

    print("Missing")
    print("\n".join(missing_modules))
    print("\n")
