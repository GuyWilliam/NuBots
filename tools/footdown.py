#!/usr/bin/env python3

import os
import tensorflow as tf
from tensorflow import keras
import yaml
from tqdm import tqdm
from util import nbs_decoder
import numpy as np

SERVO_ID = {
    "R_SHOULDER_PITCH": 0,
    "L_SHOULDER_PITCH": 1,
    "R_SHOULDER_ROLL": 2,
    "L_SHOULDER_ROLL": 3,
    "R_ELBOW": 4,
    "L_ELBOW": 5,
    "R_HIP_YAW": 6,
    "L_HIP_YAW": 7,
    "R_HIP_ROLL": 8,
    "L_HIP_ROLL": 9,
    "R_HIP_PITCH": 10,
    "L_HIP_PITCH": 11,
    "R_KNEE": 12,
    "L_KNEE": 13,
    "R_ANKLE_PITCH": 14,
    "L_ANKLE_PITCH": 15,
    "R_ANKLE_ROLL": 16,
    "L_ANKLE_ROLL": 17,
    "HEAD_YAW": 18,
    "HEAD_PITCH": 19,
}


def displacement(fk):

    # Sometimes the matrix is transposed so if x, y and z translation are 0, it's the bottom rather than the translation
    Htf = (
        np.array(
            [
                [fk.x.x, fk.x.y, fk.x.z, fk.x.t],
                [fk.y.x, fk.y.y, fk.y.z, fk.y.t],
                [fk.z.x, fk.z.y, fk.z.z, fk.z.t],
                [fk.t.x, fk.t.y, fk.t.z, fk.t.t],
            ]
        )
        if fk.t.x == 0 and fk.t.y == 0 and fk.t.z == 0
        else np.array(
            [
                [fk.x.x, fk.y.x, fk.z.x, fk.t.x],
                [fk.x.y, fk.y.y, fk.z.y, fk.t.y],
                [fk.x.z, fk.y.z, fk.z.z, fk.t.z],
                [fk.x.t, fk.y.t, fk.z.t, fk.t.t],
            ]
        )
    )

    return -(np.linalg.inv(Htf)[2, 3])


def dataset(path, state, servos, keys, foot_delta, accelerometer, gryoscope):
    xs = []
    ys = []
    for type_name, timestamp, msg in tqdm(
        nbs_decoder.decode(path), dynamic_ncols=True, unit="packet"
    ):
        if type_name == "message.input.Sensors":

            # Work out the class
            if state == "WALK":
                # Work out how far the foot is from the torso
                l_height = displacement(
                    msg.forward_kinematics[SERVO_ID["L_ANKLE_ROLL"]]
                )
                r_height = displacement(
                    msg.forward_kinematics[SERVO_ID["R_ANKLE_ROLL"]]
                )
                delta = abs(l_height - r_height)

                # Get the load on the knee
                l_load = msg.servo[SERVO_ID["R_KNEE"]].load
                r_load = msg.servo[SERVO_ID["L_KNEE"]].load

                y = (
                    1 if l_height - foot_delta < r_height else 0,
                    1 if r_height - foot_delta < l_height else 0,
                )

            elif state == "UP":
                y = (0, 0)
            elif state == "DOWN":
                y = (1, 1)
            else:
                raise RuntimeError("The state must be UP DOWN or WALK")

            x = []
            for servo in servos:
                s = msg.servo[SERVO_ID[servo]]
                values = {
                    "LOAD": s.load,
                    "POSITION": s.present_position,
                    "VELOCITY": s.present_velocity,
                }

                for key in keys:
                    x.append(values[key])
            if accelerometer:
                x.extend(
                    [msg.accelerometer.x, msg.accelerometer.y, msg.accelerometer.z]
                )
            if gryoscope:
                x.extend([msg.gyroscope.x, msg.gyroscope.y, msg.gyroscope.z])
            ys.append(y)
            xs.append(x)

    return np.array(xs), np.array(ys)


def register(command):

    # Install help
    command.help = "Train a foot down network using sensor data from the legs"

    # Drone arguments
    command.add_argument(
        "config",
        metavar="config",
        help="The configuration file specifying groups of data files",
    )


def run(config, **kwargs):

    # Load our configuration file to get the individual nbs files
    data_path = os.path.dirname(config)
    with open(config, "r") as f:
        config = yaml.safe_load(f)

    foot_delta = config["config"]["foot_delta"]
    servos = config["config"]["servos"]
    keys = config["config"]["keys"]
    use_accel = config["config"]["accelerometer"]
    use_gyro = config["config"]["gyroscope"]

    print("Loading data from NBS files")
    group_xs = []
    group_ys = []
    for group in tqdm(config["data"], dynamic_ncols=True, unit="group"):

        # Gather all groups into a single dataset for this specific type
        xs = []
        ys = []
        for f in tqdm(group["files"], dynamic_ncols=True, unit="file"):

            # Load the file
            x, y = dataset(
                os.path.join(data_path, f),
                group["state"],
                servos,
                keys,
                foot_delta,
                use_accel,
                use_gyro,
            )

            # Cut off the end 10% to account for nonsense setup and teardown
            x = x[len(x) // 10 : -len(x) // 10]
            y = y[len(y) // 10 : -len(y) // 10]

            xs.append(x)
            ys.append(y)

        group_xs.append(np.concatenate(xs, axis=0))
        group_ys.append(np.concatenate(ys, axis=0))

    # Find the largest size we have and replicate random elements in the other dataset to fill
    mx = max([x.shape[0] for x in group_xs])

    # Oversample our categories so they are the same size
    xs = []
    ys = []
    for x, y in zip(group_xs, group_ys):
        idx = np.random.randint(0, len(x), mx)
        xs.append(x[idx])
        ys.append(y[idx])
    xs = np.concatenate(xs)
    ys = np.concatenate(ys)

    # Random shuffle the data
    idx = np.arange(len(xs))
    np.random.shuffle(idx)
    xs = xs[idx]
    ys = ys[idx]

    # Split into training and validation
    split = int(len(xs) * 0.8)
    train_x = xs[:split]
    train_y = ys[:split]
    valid_x = xs[split:]
    valid_y = ys[split:]

    # Build our model
    model = keras.Sequential(
        [
            keras.layers.Dense(8, activation=tf.nn.relu),
            keras.layers.Dense(2, activation=tf.nn.sigmoid),
        ]
    )

    model.compile(
        optimizer=keras.optimizers.Adam(),
        loss=keras.losses.BinaryCrossentropy(),
        metrics=[keras.metrics.BinaryAccuracy()],
    )

    history = model.fit(
        train_x,
        train_y,
        batch_size=4096,
        epochs=1000,
        validation_data=(valid_x, valid_y),
        callbacks=[keras.callbacks.EarlyStopping(patience=5)],
    )

    print(history.history["val_binary_accuracy"][-1])
