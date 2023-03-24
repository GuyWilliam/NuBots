import * as THREE from 'three'
import { Matrix3 } from './matrix3'

import { Vector4 } from './vector4'

export class Matrix4 {
  constructor(readonly x: Vector4, readonly y: Vector4, readonly z: Vector4, readonly t: Vector4) {}

  static of() {
    return new Matrix4(
      new Vector4(1, 0, 0, 0),
      new Vector4(0, 1, 0, 0),
      new Vector4(0, 0, 1, 0),
      new Vector4(0, 0, 0, 1),
    )
  }

  static from(
    mat?: {
      x?: { x?: number | null; y?: number | null; z?: number | null; t?: number | null } | null
      y?: { x?: number | null; y?: number | null; z?: number | null; t?: number | null } | null
      z?: { x?: number | null; y?: number | null; z?: number | null; t?: number | null } | null
      t?: { x?: number | null; y?: number | null; z?: number | null; t?: number | null } | null
    } | null,
  ): Matrix4 {
    if (!mat) {
      return Matrix4.of()
    }
    return new Matrix4(
      Vector4.from(mat.x),
      Vector4.from(mat.y),
      Vector4.from(mat.z),
      Vector4.from(mat.t),
    )
  }

  static fromMatrix3(mat: Matrix3) {
    return new Matrix4(
      new Vector4(mat.x.x, mat.x.y, 0, mat.x.z),
      new Vector4(mat.y.x, mat.y.y, 0, mat.y.z),
      new Vector4(0, 0, 1, 0),
      new Vector4(mat.z.x, mat.z.y, 0, mat.z.z),
    )
  }

  get trace(): number {
    return this.x.x + this.y.y + this.z.z + this.t.t
  }

  static fromThree(mat4: THREE.Matrix4) {
    return new Matrix4(
      new Vector4(mat4.elements[0], mat4.elements[1], mat4.elements[2], mat4.elements[3]),
      new Vector4(mat4.elements[4], mat4.elements[5], mat4.elements[6], mat4.elements[7]),
      new Vector4(mat4.elements[8], mat4.elements[9], mat4.elements[10], mat4.elements[11]),
      new Vector4(mat4.elements[12], mat4.elements[13], mat4.elements[14], mat4.elements[15]),
    )
  }

  toThree(): THREE.Matrix4 {
    // prettier-ignore
    return new THREE.Matrix4().set(
      this.x.x, this.y.x, this.z.x, this.t.x,
      this.x.y, this.y.y, this.z.y, this.t.y,
      this.x.z, this.y.z, this.z.z, this.t.z,
      this.x.t, this.y.t, this.z.t, this.t.t,
    )
  }

  toString() {
    return [
      `${format(this.x.x)} ${format(this.y.x)} ${format(this.z.x)} ${format(this.t.x)}`,
      `${format(this.x.y)} ${format(this.y.y)} ${format(this.z.y)} ${format(this.t.y)}`,
      `${format(this.x.z)} ${format(this.y.z)} ${format(this.z.z)} ${format(this.t.z)}`,
      `${format(this.x.t)} ${format(this.y.t)} ${format(this.z.t)} ${format(this.t.t)}`,
    ].join('\n')
  }
}

const format = (x: number) => x.toFixed(2).padStart(7)
