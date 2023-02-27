# Localisation

## Description
A particle filter based localization method for estimating the robot's position and orientation (x, y, theta) in
field space, which relies solely on field line observations.

## Usage

Include this module to allow the robot to estimate is (x, y, theta) state on the field.

## Consumes
- `message::vision::FieldLines` uses the field line observations from FieldLineDetector module

## Emits

- `message::localisation::Field` contains the estimated (x, y, theta) state and covariance

## Dependencies

- `Eigen`
- `utility::math::stats::MultivariateNormal` Utility for sampling from a multivariate normal distribution
