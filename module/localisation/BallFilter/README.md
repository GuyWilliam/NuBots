# BallFilter

## Description

This module takes in a list of vision balls, uses the ball measurement closest to our current estimate and applies an
Unscented Kalman Filter to estimate the balls position and velocity in world space.

## Usage

Include this module to allow the robot to estimate the balls position and velocity.

## Consumes

- `message::vision::Balls` uses the ball position estimate from vision
- `message::input::Sensors` uses sensors to compute transform from camera {c} to torso space {t}

## Emits

- `message::localisation::FilteredBall` contains filtered ball position measurement

## Dependencies
