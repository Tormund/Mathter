﻿Mathter
===

![Language](https://img.shields.io/badge/Language-C++17-blue)
[![License](https://img.shields.io/badge/License-MIT-blue)](#license)
[![Build & test](https://github.com/petiaccja/Mathter/actions/workflows/build_and_test.yml/badge.svg)](https://github.com/petiaccja/Mathter/actions/workflows/build_and_test.yml)
[![Quality Gate Status](https://sonarcloud.io/api/project_badges/measure?project=petiaccja_Mathter&metric=alert_status)](https://sonarcloud.io/dashboard?id=petiaccja_Mathter)
[![Coverage](https://sonarcloud.io/api/project_badges/measure?project=petiaccja_Mathter&metric=coverage)](https://sonarcloud.io/dashboard?id=petiaccja_Mathter)


Introduction
---
Mathter is a **header-only** **linear algebra** library with focus on **game development**, however it may be useful for other applications where **small-matrix** linear algebra or **3D** coordinate calculations are needed.

**What's special about this library?** There are already many good 3D math libraries, however, most, if not all of them tie your hands with their conventions. Want a left-handed world space but a right handed NDC? Want your Z axis the other way? Want inverted or arbitrary depth? Rather multiply vectors by matrices from the left? You prefer column-major on the CPU and row-major on the GPU? You can configure mathter via templates and runtime arguments to match any convention. Additionally, Mathter provides a lot of shortcuts to reduce clutter and make your math code more expressive. Check out the code examples.

For more detailed information about using Mathter, **read the** [**guide**](https://github.com/petiaccja/Mathter/blob/master/docs/Guide.md).

Benchmark comparison with similar libraries [here](https://github.com/petiaccja/MathterBench).

Features
---
- General:
  - SIMD (for important single-precision math only, for now)
  - C++17
  - Compile-time vector size (NO runtime resize on vectors/matrices)
- Template parametrization & convention configurations:
  - T: any scalar type can be specified for Vectors, Matrices, etc. (int, float, double, complex). Of course, some of these make no sense for all operations.
  - Dimensions: vector length, matrix row and column count.
  - Order: whether transform matrices (i.e. scale, rotate) should act as v\*M or M\*v.
  - Layout: row-major or column major layout for matrices.
  - Packed: disable SIMD, forced alignment, and pack vector/matrix elements tightly in memory. Use Packed=true to handle GPU uploads.
- Mathematical primitives:
  - Vectors [v[i], v(i), v.x, swizzling: v.zyx, v.xy]
  - Matrices [m(row,col)]
  - Quaternions [q.w, q.x, q.y, q.z]
- Arithmetic:
  - Vector\*Vector
  - Vector\*Matrix
  - Vector\*Quat
  - Matrix\*Matrix
  - Quat\*Quat
- Transformations (they work in higher dimensions, too):
  - Rotation (only 2D, 3D), scale, translation, orthographic projection, perspective projection, camera look-at, shear
- Common functions:
  - Vectors: length, dot product, cross product, normalization, ...
  - Matrices: trace, determinant, inverse, norm, transpose, ...
  - Quaternions: length/abs, normalization, conjugate/inverse, exp, log, pow, ...
- Matrix decompositions:
  - LU & LUP
  - QR
  - SVD
- Geometry:
  - Lines, line segments, rays
  - Planes (hyperplanes in N-dimensions)
  - Triangles in 3D
  - Bezier curves
  - Intersections: line-hyperplane, segment-hyperplane, ray-triangle (3D), line/segment-line/segment (2D)
- Utility:
  - Math constants
  - Radians <=> degrees
  - Standard stream I/O of vectors and matrices (this is kinda hacked together TBH)
- Usage:
  - If it makes sense, it compiles (if it doesn't make sense, it doesn't compile)
  - Implicit conversions at many places, for convenience
  - Concise and powerful syntax via some C++ magic
  - Prefers mathematical notation (i.e. quaternions are chained qr = q3\*q2\*q1)
  - Many things are generalized to higher dimensions (i.e. hyperplanes, N-dimensional transforms & functions), but interfaces are not compromised

For details, browse the [API reference](https://petiaccja.github.io/Mathter).

Code example
---

Before you do anything, you want to declare types to you own taste:
```c++
using namespace mathter;
using Vec2 = Vector<float, 2, false>;
using Vec3 = Vector<float, 3, false>;
using Vec4 = Vector<float, 4, false>;
using Mat33 = Matrix<float, 3, 3, eMatrixOrder::FOLLOW_VECTOR, eMatrixLayout::ROW_MAJOR, false>;
using Mat43 = Matrix<float, 4, 3, eMatrixOrder::FOLLOW_VECTOR, eMatrixLayout::ROW_MAJOR, false>;
using Mat34 = Matrix<float, 3, 4, eMatrixOrder::FOLLOW_VECTOR, eMatrixLayout::ROW_MAJOR, false>;
using Mat44 = Matrix<float, 4, 4, eMatrixOrder::FOLLOW_VECTOR, eMatrixLayout::ROW_MAJOR, false>;
using Quat = Quaternion<float, false>;
```
Remember, all stuff is in the namespace mathter.

You have multiple ways to initialize your variables. Beware, **they are not null-initialized** for performance and safety reasons. (Read more about initialization and changing the behaviour in the [guide's configuration section](https://github.com/petiaccja/Mathter/blob/master/docs/Guide.md#configuration)).
```c++
Vec2 a = { 1, 2 };
Vec3 v1 = { 1, 2, 3 };
Vec3 v2 = { a, 3 };
Vec4 v3 = a.xxyy;
Mat33 mdecl = {
  1, 0, 0,
  0, 1, 0,
  0, 0, 1,
};
```

Transforms are for the most part represented by matrices. You can use the builder functions to construct them. It is important that the builder functions don't create any matrices, you have to assign them to a matrix with a well-defined type. As long as the destination matrix can represent the transform, you'll be fine:
```c++
Mat44 m = Translation(3, 2, 1); // Always fine, but the 3 values are placed differently with FOLLOW vs. PRECEDE.
Mat43 m43 = Translation(3, 2, 1); // This is FOLLOW_VECTOR, fine.
// Mat34 m34 = Translation(3, 2, 1); // Doesn't compile. This should be PRECEDE_VECTOR.
```

You can multiply vectors and matrices fairly liberally. The first two operations extend the 3-vector to a 4-vector by appending a 1 at the end, then multiply by the 4x4 matrix, do the perspective division with the last element of the resulting 4-vector, then truncate the result to a 3-vector. The last line does this manually, without perspective division.
```c++
Vec3 r11 = v1 * m;
Vec3 r12 = Transpose(m)*v1;
Vec3 r2 = Vec3((v2 | 1) * m);
```

The rotation builders work much the same for matrices and quaternions:
```c++
Quat q = RotationAxisAngle(Normalize(Vec3(1, 2, 3)), Deg2Rad(60.f));
Mat33 mr = RotationAxisAngle(Normalize(Vec3(1, 2, 3)), Deg2Rad(60.f));
mr = Identity();
q = Identity();
```

And you can also convert between the 2 representations:
```c++
mr = Mat33(q);
q = Quat(mr);
```

Applying rotations to vectors with quaternions is straightforward, but in general it's faster with matrices:
```c++
Vec3 vr = v1 * q;
```

You can also do other things like solving linear equations via an LUP decomposition of the matrix. Apart from rotations, things work in higher dimensions, too:
```c++
Vector<float, 6> b = { 1, 2, 3, 4, 5, 6 };
Matrix<float, 6, 6> M;
for (int row = 0; row < M.RowCount(); ++row) {
	for (int col = 0; col < M.RowCount(); ++col) {
		M(row, col) = rand() % 1000 / 1000.f;
	}
}
Vector<float, 6> x = DecomposeLUP(M).Solve(b); // Mx = b
```

Installation
---
**From the conan center:** https://conan.io/center/mathter.

**Manually:** Mathter is header-only, you just have to **copy the \<repo\>/Mathter** folder into your include path.

**Compilers:** tested for GCC, Clang and MSVC via GitHub CI. Enabling std=c++17 is required.

**Visual Studio:** don't forget to add the file Mathter/Mathter.natvis to your visual studio projects, it will display Mathter types nicely in the debugger.

License
---
The code is using the **MIT license**, which is a very permissive license suitable for non-commercial and commercial uses of Mathter alike. However, you have to include the copyright notice in your code. Read the full license for the exact terms.