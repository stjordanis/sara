// ========================================================================== //
// This file is part of Sara, a basic set of libraries in C++ for computer
// vision.
//
// Copyright (C) 2013-2016 David Ok <david.ok8@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
// ========================================================================== //

//! @file
//! @todo Refactor this file. It needs code review.

#pragma once

#include <cmath>

#include <DO/Sara/Core/EigenExtension.hpp>


namespace DO { namespace Sara {

  //! @addtogroup GeometryTools
  //! @{

  //! @{
  template <typename T>
  inline int int_round(T x)
  {
    return static_cast<int>(std::round(x));
  }
  //! @}

  //! @brief Sign function.
  template <typename T>
  inline int signum(T val)
  {
    return (T(0) < val) - (val < T(0));
  }

  //! @brief Degree to radian conversion.
  template <typename T>
  inline T to_radian(T degree)
  {
    static_assert(!std::numeric_limits<T>::is_integer,
                  "Scalar must be of floating type");
    return degree * static_cast<T>(M_PI) / static_cast<T>(180);
  }

  //! @brief Radian to degree conversion.
  template <typename T>
  inline T to_degree(T radian)
  {
    static_assert(!std::numeric_limits<T>::is_integer,
                  "Scalar must be of floating type");
    return radian * static_cast<T>(180) / static_cast<T>(M_PI);
  }

  //! @{
  //! @brief Check if the basis [u, v] is counter-clockwise.
  template <typename T>
  inline T cross(const Matrix<T, 2, 1>& u, const Matrix<T, 2, 1>& v)
  {
    Matrix<T, 2, 2> M;
    M.col(0) = u;
    M.col(1) = v;
    return M.determinant();
  }

  template <typename T>
  inline T cross(const Matrix<T, 2, 1>& a, const Matrix<T, 2, 1>& b,
                 const Matrix<T, 2, 1>& c)
  {
    return cross((b - a).eval(), (c - a).eval());
  }
  //! @}

  /*!
    Suppose the 'b-a' is an upfront vector.
    There are three cases:
    - If point 'c' is on the left, then det([b-a, c-a]) > 0.
    - If point 'c' is on the right, then det([b-a, c-a]) < 0.
    - If point 'c' is on the line (a,b), then det([b-a, c-a]) = 0.
    */
  template <typename T>
  inline int ccw(const Matrix<T, 2, 1>& a, const Matrix<T, 2, 1>& b,
                 const Matrix<T, 2, 1>& c)
  {
    return signum(cross(a, b, c));
  }

  template <typename T>
  inline Matrix<T, 2, 1> unit_vector2(T radian)
  {
    static_assert(!std::numeric_limits<T>::is_integer,
                  "Scalar must be of floating type");
    return Matrix<T, 2, 1>(std::cos(radian), std::sin(radian));
  }

  //! @{
  //! @brief Geometric transforms.
  template <typename T>
  inline Matrix<T, 2, 2> rotation2(T radian)
  {
    static_assert(!std::numeric_limits<T>::is_integer,
                  "Scalar must be of floating type");
    return Eigen::Rotation2D<T>(radian).toRotationMatrix();
  }

  template <typename T>
  inline Matrix<T, 2, 2> isometry2(T radian, T scale)
  {
    static_assert(!std::numeric_limits<T>::is_integer,
                  "Scalar must be of floating type");
    return Eigen::Rotation2D<T>(radian).toRotationMatrix()*scale;
  }

  template <typename T>
  Matrix<T, 2, 2> linear_transform_2(const Matrix<T, 2, 1>& p1,
                                     const Matrix<T, 2, 1>& p2,
                                     const Matrix<T, 2, 1>& q1,
                                     const Matrix<T, 2, 1>& q2)
  {
    Matrix<T, 4, 4> M;
    M << p1.x(), p1.y(),   T(0),   T(0),
           T(0),   T(0), p1.x(), p1.y(),
         p2.x(), p2.y(),   T(0),   T(0),
           T(0),   T(0), p2.x(), p2.y();

    Matrix<T, 4, 1> b;
    b << q1.x(), q1.y(), q2.x(), q2.y();

    Matrix<T, 4, 1> vecA(M.colPivHouseholderQr().solve(b));

    Matrix<T, 2, 2> A;
    A << vecA[0], vecA[1],
         vecA[2], vecA[3];

    return A;
  }

  template <typename T>
  Matrix<T, 3, 3> affine_transform_2(const Matrix<T, 2, 1>& p1,
                                     const Matrix<T, 2, 1>& p2,
                                     const Matrix<T, 2, 1>& p3,
                                     const Matrix<T, 2, 1>& q1,
                                     const Matrix<T, 2, 1>& q2,
                                     const Matrix<T, 2, 1>& q3)
  {
    Matrix<T, 6, 6> M;
    M << p1.x(), p1.y(), T(1),   T(0),   T(0), T(0),
           T(0),   T(0), T(0), p1.x(), p1.y(), T(1),
         p2.x(), p2.y(), T(1),   T(0),   T(0), T(0),
           T(0),   T(0), T(0), p2.x(), p2.y(), T(1),
         p3.x(), p3.y(), T(1),   T(0),   T(0), T(0),
           T(0),   T(0), T(0), p3.x(), p3.y(), T(1);

    Matrix<T, 6, 1> y;
    y << q1.x(), q1.y(), q2.x(), q2.y(), q3.x(), q3.y();

    Matrix<T, 6, 1> x(M.colPivHouseholderQr().solve(y));

    Matrix<T, 3, 3> A;
    A << x[0], x[1], x[2],
         x[3], x[4], x[5],
         T(0), T(0), T(1);

    return A;
  }

  template <typename T>
  Matrix<T, 2, 2> linear_part_from_affinity(const Matrix<T, 3, 3>& A)
  {
    return A.template block<2,2>(0,0);
  }

  template <typename T>
  Matrix<T, 3, 3> homography(const Matrix<T, 2, 1>& p1,
                             const Matrix<T, 2, 1>& p2,
                             const Matrix<T, 2, 1>& p3,
                             const Matrix<T, 2, 1>& p4,
                             const Matrix<T, 2, 1>& q1,
                             const Matrix<T, 2, 1>& q2,
                             const Matrix<T, 2, 1>& q3,
                             const Matrix<T, 2, 1>& q4)
  {
    Matrix<T, 8, 8> M;
    M <<
    p1.x(), p1.y(), T(1),   T(0),   T(0), T(0), -p1.x()*q1.x(), -p1.y()*q1.x(),
      T(0),   T(0), T(0), p1.x(), p1.y(), T(1), -p1.x()*q1.y(), -p1.y()*q1.y(),
    p2.x(), p2.y(), T(1),   T(0),   T(0), T(0), -p2.x()*q2.x(), -p2.y()*q2.x(),
      T(0),   T(0), T(0), p2.x(), p2.y(), T(1), -p2.x()*q2.y(), -p2.y()*q2.y(),
    p3.x(), p3.y(), T(1),   T(0),   T(0), T(0), -p3.x()*q3.x(), -p3.y()*q3.x(),
      T(0),   T(0), T(0), p3.x(), p3.y(), T(1), -p3.x()*q3.y(), -p3.y()*q3.y(),
    p4.x(), p4.y(), T(1),   T(0),   T(0), T(0), -p4.x()*q4.x(), -p4.y()*q4.x(),
      T(0),   T(0), T(0), p4.x(), p4.y(), T(1), -p4.x()*q4.y(), -p4.y()*q4.y();

    Matrix<T, 8, 1> y;
    y << q1.x(), q1.y(), q2.x(), q2.y(), q3.x(), q3.y(), q4.x(), q4.y();

    Matrix<T, 8, 1> x(M.colPivHouseholderQr().solve(y));

    Matrix<T, 3, 3> A;
    A << x[0], x[1], x[2],
         x[3], x[4], x[5],
         x[6], x[7], T(1);

    return A;
  }

  template <typename T>
  Matrix<T, 2, 2> homography_jacobian_matrix(const Matrix<T, 3, 3>& H,
                                             const Matrix<T, 2, 1>& x)
  {
    const auto u = H(0,0)*x[0] + H(0,1)*x[1] + H(0,2);
    const auto v = H(1,0)*x[0] + H(1,1)*x[1] + H(1,2);
    const auto w = H(2,0)*x[0] + H(2,1)*x[1] + H(2,2);

    auto dH = Matrix<T, 2, 2>{};
    dH <<  //
        (H(0, 0) * w - H(2, 0) * u) / (w * w),
        (H(1, 0) * w - H(2, 0) * u) / (w * w),
        (H(0, 1) * w - H(2, 1) * u) / (w * w),
        (H(1, 1) * w - H(2, 1) * u) / (w * w);

    return dH;
  }
  //! @}

  //! @brief Utility function.
  template <typename T>
  Matrix<T, 2, 1> apply(const Matrix<T, 3, 3>& H, const Matrix<T, 2, 1>& p)
  {
    return (H * p.homogeneous()).hnormalized();
  }

  //! @}

} /* namespace Sara */
} /* namespace DO */
