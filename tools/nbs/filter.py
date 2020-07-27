#!/usr/bin/env python3

import fnmatch

from tqdm import tqdm

from .nbs import Decoder, Encoder


def register(command):
    command.help = (
        "Filter an nbs stream, removing messages from it. ",
        "The command will first remove any patterns provided with -r ",
        "(or everything if no remove patterns are given) ",
        "and then keep any of those removed that match the -k patterns",
        "for example to keep only Sensors and DarwinSensors ",
        "`./b nbs filter -k message.input.Sensors -k message.platform ",
        "`or to remove all CompressedImages `./b nbs filter -r message.output.CompressedImage",
    )

    # Command arguments
    command.add_argument("files", metavar="files", nargs="+", help="The nbs files to filter")
    command.add_argument("-k", "--keep", action="append", help="A message pattern to keep")
    command.add_argument("-r", "--remove", action="append", help="A message pattern to remove")
    command.add_argument("-o", "--output", default="out.nbs", help="The output file to store the filtered nbs in")


def run(files, keep, remove, output, **kwargs):

    # If no remove was provided, we assume we remove all
    if remove is None:
        if keep is None:
            print("You must provide either something to remove or keep")
            exit(1)
        else:
            remove = ["*"]

    # If we didn't provide any keep arguments we at least need to ignore them
    if keep is None:
        keep = []

    with Encoder(output) as out:

        decoder = Decoder(*files)
        with tqdm(total=len(decoder), unit="B", unit_scale=True, dynamic_ncols=True) as progress:
            for packet in decoder:

                # To start with we keep messages
                valid = True

                # If remove flags any message mark it as invalid
                for pattern in remove:
                    if fnmatch.fnmatch(packet.type, pattern):
                        valid = False

                # If keep flags any message mark it as valid
                for pattern in keep:
                    if fnmatch.fnmatch(packet.type, pattern):
                        valid = True

                # If it's still valid write it to the new file
                if valid:
                    out.write(packet.timestamp, packet.msg)

                # Update the progress bar
                progress.n = decoder.bytes_read()
                progress.update(0)
