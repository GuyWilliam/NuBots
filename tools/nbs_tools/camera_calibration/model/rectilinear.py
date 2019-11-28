#!/usr/bin/env python3

import tensorflow as tf


class Rectilinear(tf.keras.Model):
    def __init__(self, focal_length, centre, **kwargs):
        super(Rectilinear, self).__init__()

        self.focal_length = tf.Variable(initial_value=1, dtype=tf.float32)
        self.centre = tf.Variable(initial_value=[0, 0], dtype=tf.float32)

        self.focal_length.assign(focal_length)
        self.centre.assign(centre)

    def call(self, screen, training=False):

        offset = tf.add(screen, self.centre)

        return tf.linalg.normalize(
            tf.stack([tf.ones_like(offset[..., 0]) * self.focal_length, offset[..., 0], offset[..., 1]], axis=-1),
            axis=-1,
        )[0]
