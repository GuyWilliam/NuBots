import { observable } from "mobx";
import { computed } from "mobx";

import { Matrix4 } from "../../../shared/math/matrix4";
import { Quaternion } from "../../../shared/math/quaternion";
import { Vector3 } from "../../../shared/math/vector3";
import { memoize } from "../../base/memoize";
import { RobotModel } from "../robot/model";

class ServoMotor {
  @observable angle: number;

  constructor({ angle }: ServoMotor) {
    this.angle = angle;
  }

  static of() {
    return new ServoMotor({ angle: 0 });
  }
}

export class ServoMotorSet {
  @observable rightShoulderPitch: ServoMotor;
  @observable leftShoulderPitch: ServoMotor;
  @observable rightShoulderRoll: ServoMotor;
  @observable leftShoulderRoll: ServoMotor;
  @observable rightElbow: ServoMotor;
  @observable leftElbow: ServoMotor;
  @observable rightHipYaw: ServoMotor;
  @observable leftHipYaw: ServoMotor;
  @observable rightHipRoll: ServoMotor;
  @observable leftHipRoll: ServoMotor;
  @observable rightHipPitch: ServoMotor;
  @observable leftHipPitch: ServoMotor;
  @observable rightKnee: ServoMotor;
  @observable leftKnee: ServoMotor;
  @observable rightAnklePitch: ServoMotor;
  @observable leftAnklePitch: ServoMotor;
  @observable rightAnkleRoll: ServoMotor;
  @observable leftAnkleRoll: ServoMotor;
  @observable headPan: ServoMotor;
  @observable headTilt: ServoMotor;

  constructor({
    rightShoulderPitch,
    leftShoulderPitch,
    rightShoulderRoll,
    leftShoulderRoll,
    rightElbow,
    leftElbow,
    rightHipYaw,
    leftHipYaw,
    rightHipRoll,
    leftHipRoll,
    rightHipPitch,
    leftHipPitch,
    rightKnee,
    leftKnee,
    rightAnklePitch,
    leftAnklePitch,
    rightAnkleRoll,
    leftAnkleRoll,
    headPan,
    headTilt,
  }: ServoMotorSet) {
    this.rightShoulderPitch = rightShoulderPitch;
    this.leftShoulderPitch = leftShoulderPitch;
    this.rightShoulderRoll = rightShoulderRoll;
    this.leftShoulderRoll = leftShoulderRoll;
    this.rightElbow = rightElbow;
    this.leftElbow = leftElbow;
    this.rightHipYaw = rightHipYaw;
    this.leftHipYaw = leftHipYaw;
    this.rightHipRoll = rightHipRoll;
    this.leftHipRoll = leftHipRoll;
    this.rightHipPitch = rightHipPitch;
    this.leftHipPitch = leftHipPitch;
    this.rightKnee = rightKnee;
    this.leftKnee = leftKnee;
    this.rightAnklePitch = rightAnklePitch;
    this.leftAnklePitch = leftAnklePitch;
    this.rightAnkleRoll = rightAnkleRoll;
    this.leftAnkleRoll = leftAnkleRoll;
    this.headPan = headPan;
    this.headTilt = headTilt;
  }

  static of() {
    return new ServoMotorSet({
      rightShoulderPitch: ServoMotor.of(),
      leftShoulderPitch: ServoMotor.of(),
      rightShoulderRoll: ServoMotor.of(),
      leftShoulderRoll: ServoMotor.of(),
      rightElbow: ServoMotor.of(),
      leftElbow: ServoMotor.of(),
      rightHipYaw: ServoMotor.of(),
      leftHipYaw: ServoMotor.of(),
      rightHipRoll: ServoMotor.of(),
      leftHipRoll: ServoMotor.of(),
      rightHipPitch: ServoMotor.of(),
      leftHipPitch: ServoMotor.of(),
      rightKnee: ServoMotor.of(),
      leftKnee: ServoMotor.of(),
      rightAnklePitch: ServoMotor.of(),
      leftAnklePitch: ServoMotor.of(),
      rightAnkleRoll: ServoMotor.of(),
      leftAnkleRoll: ServoMotor.of(),
      headPan: ServoMotor.of(),
      headTilt: ServoMotor.of(),
    });
  }
}

export class LocalisationRobotModel {
  @observable private model: RobotModel;
  @observable name: string;
  @observable color?: string;
  @observable Htw: Matrix4; // World to torso
  @observable Hfw: Matrix4; // World to field
  @observable Hwp: Matrix4; // Anchor point (planted foot) to world
  @observable Rwt: Quaternion; // Torso to world rotation.
  @observable motors: ServoMotorSet;
  @observable fieldLinePoints: { rPWw: Vector3[] };
  @observable ball?: { rBWw: Vector3 };
  @observable swingFootTrajectory: { rSPp: Vector3[] };
  @observable swingFootTrajectoryHistory: { trajectories: { trajectory: Vector3[]; walkPhase: number }[] };
  @observable torsoTrajectory: { rTPp: Vector3[] };
  @observable torsoTrajectoryHistory: { trajectories: Vector3[][] };
  @observable walkPhase: number;

  constructor({
    model,
    name,
    color,
    Htw,
    Hfw,
    Hwp,
    Rwt,
    motors,
    fieldLinePoints,
    ball,
    swingFootTrajectory,
    swingFootTrajectoryHistory,
    torsoTrajectory,
    torsoTrajectoryHistory,
    walkPhase,
  }: {
    model: RobotModel;
    name: string;
    color?: string;
    Htw: Matrix4;
    Hfw: Matrix4;
    Hwp: Matrix4;
    Rwt: Quaternion;
    motors: ServoMotorSet;
    fieldLinePoints: { rPWw: Vector3[] };
    ball?: { rBWw: Vector3 };
    swingFootTrajectory: { rSPp: Vector3[] };
    swingFootTrajectoryHistory: { trajectories: { trajectory: Vector3[]; walkPhase: number }[] };
    torsoTrajectory: { rTPp: Vector3[] };
    torsoTrajectoryHistory: { trajectories: Vector3[][] };
    walkPhase: number;
  }) {
    this.model = model;
    this.name = name;
    this.color = color;
    this.Htw = Htw;
    this.Hfw = Hfw;
    this.Hwp = Hwp;
    this.Rwt = Rwt;
    this.motors = motors;
    this.fieldLinePoints = fieldLinePoints;
    this.ball = ball;
    this.swingFootTrajectory = swingFootTrajectory;
    this.swingFootTrajectoryHistory = swingFootTrajectoryHistory;
    this.torsoTrajectory = torsoTrajectory;
    this.torsoTrajectoryHistory = torsoTrajectoryHistory;
    this.walkPhase = walkPhase;
  }

  static of = memoize((model: RobotModel): LocalisationRobotModel => {
    return new LocalisationRobotModel({
      model,
      name: model.name,
      Htw: Matrix4.of(),
      Hfw: Matrix4.of(),
      Hwp: Matrix4.of(),
      Rwt: Quaternion.of(),
      motors: ServoMotorSet.of(),
      fieldLinePoints: { rPWw: [] },
      swingFootTrajectory: { rSPp: [] },
      swingFootTrajectoryHistory: { trajectories: [] },
      torsoTrajectory: { rTPp: [] },
      torsoTrajectoryHistory: { trajectories: [] },
      walkPhase: 0,
    });
  });

  @computed get id() {
    return this.model.id;
  }

  @computed get visible() {
    return this.model.enabled;
  }

  /** Torso to field transformation */
  @computed
  get Hft(): Matrix4 {
    return this.Hfw.multiply(this.Htw.invert());
  }

  /** Anchor point to field transformation */
  @computed
  get Hfp(): Matrix4 {
    return this.Hfw.multiply(this.Hwp);
  }

  /** Field line points in field space */
  @computed
  get rPFf(): Vector3[] {
    return this.fieldLinePoints.rPWw.map((rPWw) => rPWw.applyMatrix4(this.Hfw));
  }

  /** Ball position in field space */
  @computed
  get rBFf(): Vector3 | undefined {
    return this.ball?.rBWw.applyMatrix4(this.Hfw);
  }

  /** Swing foot trajectory in field space */
  @computed
  get rSFf(): Vector3[] {
    return this.swingFootTrajectory.rSPp.map((rSPp) => rSPp.applyMatrix4(this.Hfp));
  }

  /** Torso trajectory in field space */
  @computed
  get rTFf(): Vector3[] {
    return this.torsoTrajectory.rTPp.map((rTPp) => rTPp.applyMatrix4(this.Hfp));
  }
}
