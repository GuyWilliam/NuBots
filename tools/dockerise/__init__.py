#!/usr/bin/env python3

import os
import b
import re
import sys
import shutil
import subprocess
import array
import fcntl
import pty
import tty
import signal
import termios
from tqdm import tqdm

# Try to import docker, if we are already in docker this will fail
try:
    import docker
    import dockerpty

    client = docker.from_env()
except:
    client = None

repository = "nubots"
user = "nubots"
directory = "NUbots"


# Based on the implementation at https://gist.github.com/jathanism/4489235
class WrapPty:
    def __init__(self):
        self.fd = None

    def _set_pty_size(self):
        buf = array.array("h", [0, 0, 0, 0])
        fcntl.ioctl(pty.STDOUT_FILENO, termios.TIOCGWINSZ, buf, True)
        fcntl.ioctl(self.fd, termios.TIOCSWINSZ, buf)

    def _signal_winch(self, signum, frame):
        """
        Signal handler for SIGWINCH - window size has changed.
        """
        self._set_pty_size()

    def spawn(self, args):
        pid, self.fd = pty.fork()

        if len(args) == 0:
            args = ["/bin/bash"]

        if pid == pty.CHILD:
            # If no arguments run bash
            os.execlp(args[0], *args)

        old_handler = signal.signal(signal.SIGWINCH, self._signal_winch)
        try:
            mode = tty.tcgetattr(pty.STDIN_FILENO)
            tty.setraw(pty.STDIN_FILENO)
            restore = True
        except tty.error:  # This is the same as termios.error
            restore = False

        self._set_pty_size()

        try:
            pty._copy(self.fd)
        except (IOError, OSError):
            if restore:
                tty.tcsetattr(pty.STDIN_FILENO, tty.TCSAFLUSH, mode)

        os.close(self.fd)
        signal.signal(signal.SIGWINCH, old_handler)
        self.fd = None
        return os.waitpid(pid, 0)[1] >> 8


def is_docker():
    path = "/proc/self/cgroup"
    return os.path.exists("/.dockerenv") or os.path.isfile(path) and any("docker" in line for line in open(path))


def build_platform(platform):
    tag = "{}:{}".format(repository, platform)
    dockerdir = os.path.join(b.project_dir, "docker")

    # Pull the latest version from dockerhub
    progress = {}
    for event in client.api.pull(repository="nubots/nubots", tag=platform, stream=True, decode=True):
        try:
            id = int(event["id"], 16)
            status = event["status"]

            # If this is the first time we have seen this id, make a tqdm progress bar for it
            if id not in progress:
                progress[id] = {"bar": tqdm(unit="B", unit_scale=True, dynamic_ncols=True, leave=False), "value": 0}

            p = progress[id]
            bar = p["bar"]

            # Update the status
            bar.set_description("{} - {}".format(id, status))

            # If we have a value in progressDetail
            if (
                "progressDetail" in event
                and "current" in event["progressDetail"]
                and "total" in event["progressDetail"]
            ):
                current = int(event["progressDetail"]["current"], 10)
                total = int(event["progressDetail"]["total"], 10)

                bar.total = total
                bar.n = current

            # Complete statuses need to finish off the bar
            if "complete" in status:
                bar.finish()

        except KeyError:
            print(event["status"])
        except ValueError:
            print(event["status"])

    # Build the image
    for event in client.api.build(
        path=dockerdir,
        tag=tag,
        buildargs={"platform": platform},
        quiet=False,
        pull=True,
        rm=True,
        decode=True,
        cache_from=["nubots:{}".format(platform), "nubots/nubots:{}".format(platform)],
    ):
        if "stream" in event:
            sys.stdout.write(event["stream"])


def platforms():
    # Get the possible platforms
    p = re.compile("generate_([a-z0-9]+)_toolchain.py")
    return [
        m.group(1)
        for m in [p.match(s) for s in os.listdir(os.path.join(b.project_dir, "docker", "usr", "local", "toolchain"))]
        if m is not None
    ]


def get_selected_platform():
    try:
        selected = client.images.get("{}:selected".format(repository))
        names = [
            t.split(":")[-1]
            for t in selected.tags
            if t != "{}:selected".format(repository) and t.startswith("{}:".format(repository))
        ]
        if len(names) == 0:
            print("ERROR the currently selected platform is a dangling tag.")
            print("      The system is unable to work out what platform this was and will need to be reset")
            print("      run `./b target {platform}` to correct this")
            exit(1)
        elif len(names) == 1:
            return names[0]
        else:
            print("WARNING There are multiple platforms with the same image tag.")
            print("        The possible tags are [{}]".format(", ".join(names)))
            platform = list(sorted(names))[0]
            print("        The platform chosen will be {}".format(platform))
            return platform
    except docker.errors.ImageNotFound:
        print("No platform has been selected yet, defaulting to generic")
        return "generic"


def run_on_docker(func):
    if func.__name__ == "register":

        def register(command):

            # Add our docker specific options
            command.add_argument("--rebuild", action="store_true", help="rebuild the docker image checking for updates")
            command.add_argument(
                "--clean", action="store_true", help="delete and recreate all docker volumes (build directories)"
            )
            command.add_argument(
                "--platform",
                choices=platforms(),
                default="selected",
                nargs="?",
                help="The image to use for the docker container",
            )

            func(command)

        return register

    elif func.__name__ == "run":

        def run(rebuild, clean, platform, **kwargs):
            # If we are running in docker, then execute the command as normal
            if is_docker():
                func(rebuild=rebuild, clean=clean, platform=platform, **kwargs)
            # Otherwise go and re-run the b script in docker
            else:
                # If the platform was "selected" that means to use the currently selected platform
                selected_platform = platform == "selected"
                if selected_platform:
                    platform = get_selected_platform()

                # Check if the image we want exists
                tag = "{}:{}".format(repository, platform)
                try:
                    client.images.get(tag)
                except docker.errors.ImageNotFound:
                    print("Could not find the image {}, rebuilding from source".format(tag))
                    rebuild = True

                # If we are requesting a rebuild, then run build
                if rebuild:
                    build_platform(platform)
                    # If we were building the selected platform we have to move our selected tag up
                    if selected_platform:
                        client.images.get(tag).tag(repository, "selected")

                # Find the volume for this platform
                build_volume_name = "{}_{}_build".format(repository, platform)
                try:
                    build_volume = client.volumes.get(build_volume_name)

                    # If we are cleaning, remove this volume so we can recreate it
                    if clean:
                        build_volume.remove(force=True)
                        build_volume = None
                except docker.errors.NotFound:
                    build_volume = None

                # If we don't have a volume, make one
                if build_volume is None:
                    build_volume = client.volumes.create(build_volume_name)

                # Work out what cwd we need to have on docker to mirror the cwd we have here
                code_to_cwd = os.path.relpath(os.getcwd(), b.project_dir)
                cwd_to_code = os.path.relpath(b.project_dir, os.getcwd())

                # If we are running in WSL we need to translate our path to a windows one for docker
                bind_path = (
                    b.project_dir
                    if shutil.which("wslpath") is None
                    else subprocess.check_output(["wslpath", "-m", b.project_dir])[:-1].decode("utf-8")
                )

                container = client.containers.create(
                    tag,
                    command=["{}/b".format(cwd_to_code), *sys.argv[1:]],
                    working_dir="/home/{}/{}/{}".format(user, directory, code_to_cwd),
                    auto_remove=True,
                    tty=True,
                    stdin_open=True,
                    hostname="docker",
                    network_mode="host",
                    mounts=[
                        docker.types.Mount(
                            type="bind",
                            source=bind_path,
                            target="/home/{}/{}".format(user, directory),
                            consistency="cached",
                        ),
                        docker.types.Mount(
                            type="volume",
                            source=build_volume.name,
                            target="/home/{}/build".format(user),
                            consistency="delegated",
                        ),
                    ],
                )

                # Attach a pty to this terminal
                dockerpty.start(client.api, container.id)
                exit(container.wait()["StatusCode"])

        return run

    else:
        raise RuntimeError("run_on_docker must only be applied to the run and register functions")
