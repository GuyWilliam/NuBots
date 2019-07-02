#!/usr/bin/env python3


def register(command):

    # Install help
    command.help = "Decode an nbs file into a series of json objects"

    # Drone arguments
    command.add_argument("path", metavar="path", help="The file to decode into a series of json objects")


def run(path, **kwargs):

    # Import the nbs decoder
    import re
    from util import nbs_decoder
    from google.protobuf.json_format import MessageToJson

    for type_name, timestamp, msg in nbs_decoder.decode(path):
        out = re.sub(r"\s+", " ", MessageToJson(msg, True))
        out = '{{ "type": "{}", "timestamp": {}, "data": {} }}'.format(type_name, timestamp, out)
        # Print as a json object
        print(out)
