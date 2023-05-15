import { computed } from "mobx";
import * as THREE from "three";

import { Matrix4 } from "../../../math/matrix4";
import { Vector3 } from "../../../math/vector3";
import { Vector4 } from "../../../math/vector4";
import { group } from "../../three/builders";
import { Canvas } from "../../three/three";

import { LineProjection } from "./line_projection";
import { GreenHorizon } from "./model";
import { CameraParams } from "./model";

export class GreenHorizonViewModel {
  private readonly greenHorizon: GreenHorizon;
  private readonly params: CameraParams;
  private readonly lineProjection: LineProjection;

  constructor(greenHorizon: GreenHorizon, params: CameraParams, lineProjection: LineProjection) {
    this.greenHorizon = greenHorizon;
    this.params = params;
    this.lineProjection = lineProjection;
  }

  static of(canvas: Canvas, greenHorizon: GreenHorizon, params: CameraParams): GreenHorizonViewModel {
    return new GreenHorizonViewModel(greenHorizon, params, LineProjection.of(canvas, params.lens));
  }

  readonly greenhorizon = group(() => ({
    children: this.greenHorizon.horizon.map((_, index) => {
      // For n given rays there are n - 1 line segments between them.
      return index >= 1
        ? this.lineProjection.planeSegment({
            start: this.rays[index - 1],
            end: this.rays[index],
            color: new Vector4(0, 0.8, 0, 0.8),
            lineWidth: 10,
          })
        : undefined;
    }),
  }));

  @computed
  private get rays() {
    // This method transforms the green horizon rays such that they track correctly with the latest camera image.
    //
    // e.g. If we have a 5 second old green horizon measurement, and receive a new camera image, the image may have
    // shifted perspective, and the green horizon would look incorrectly offset on screen.
    //
    // We transform the rays by using the Hcw from the green horizon measurement and the Hcw from the camera image
    // measurement. We firstly project the rays down to the ground using the height from the green horizon Hcw
    // We then transform the ground points into world space by adding the translation to that camera.
    // Once we have ground points in the world space, we use any Hcw matrix to transform them to that camera's view.
    // This effectively remaps the rays to the perspective of the new camera image.

    const { horizon, Hcw: greenHorizonHcw } = this.greenHorizon;
    const imageHcw = this.params.Hcw;
    const greenHorizonHwc = Matrix4.fromThree(new THREE.Matrix4().getInverse(greenHorizonHcw.toThree()));
    const rCWw = greenHorizonHwc.t.vec3();
    return horizon.map((ray) =>
      Vector3.fromThree(
        ray
          .toThree() // rUCw
          // Project world space unit vector onto the world/field ground, giving us a camera to field vector in world space.
          .multiplyScalar(ray.z !== 0 ? -greenHorizonHwc.t.z / ray.z : 1) // rFCw
          // Get the world to field vector, so that we can...
          .add(rCWw.toThree()) // rFWw = rFCw + rCWw
          // ...apply the camera image's world to camera transform, giving us a corrected camera space vector.
          .applyMatrix4(imageHcw.toThree()) // rFCc
          // Normalize to get the final camera space direction vector/ray.
          .normalize(), // rUCc
      ),
    );
  }
}
