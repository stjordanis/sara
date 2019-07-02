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

#pragma once

#include <DO/Sara/Defines.hpp>

#include <DO/Sara/Core/EigenExtension.hpp>
#include <DO/Sara/Core/Pixel.hpp>

#include <cstdint>
#include <stdexcept>


namespace DO { namespace Sara {

  /*!
    @ingroup Features
    @{
  */

  /*!
    The 'OERegion' class stands for 'Oriented Elliptic Region' and is
    dedicated to store important geometric features such as:
    - DoG
    - Harris-Affine
    - Hessian-Affine features and so on...
   */
  class DO_SARA_EXPORT OERegion
  {
  public:
    //! @{
    //! @brief Feature type.
    enum class Type : std::int8_t
    {
      Harris,
      HarAff,
      HarLap,
      FAST,
      SUSAN,
      DoG,
      LoG,
      DoH,
      MSER,
      HesAff,
      HesLap
    };

    enum class ExtremumType : std::int8_t
    {
      Min = -1,
      Saddle = 0,
      Max = 1
    };
    //! @}

    //! @brief Default constructor
    OERegion() = default;

    //! @brief Constructor for circular region.
    OERegion(const Point2f& coords)
      : coords(coords)
    {
    }

    OERegion(const Point2f& coords, float scale)
      : coords{coords}
      , shape_matrix{Matrix2f::Identity() * (pow(scale, -2))}
    {
    }

    //! @{
    //! @brief Convenient getters.
    auto x() const -> float
    {
      return coords(0);
    }

    auto x()-> float&
    {
      return coords(0);
    }

    auto y() const -> float
    {
      return coords(1);
    }

    auto y() -> float&
    {
      return coords(1);
    }

    auto center() const -> const Point2f&
    {
      return coords;
    }

    auto center() -> Point2f&
    {
      return coords;
    }
    //! @}

    //! @brief Return the anisotropic radius at a given angle in radians.
    float radius(float radian = 0.f) const;

    //! @brief Return the anisotropic scale at a given angle in radians.
    float scale(float radian = 0.f) const
    {
      return radius(radian);
    }

    //! Get the affine transform $A$ that transforms the unit circle to that
    //! oriented ellipse of the region.
    //! We compute $A$ from a QR decomposition and by observing
    //! $M = (A^{-1})^T A^{-1}$ where $M$ is the shape matrix.
    Matrix3f affinity() const;

    //! @brief Compare two regions.
    bool operator==(const OERegion& other) const
    {
      return (coords == other.coords &&              //
              shape_matrix == other.shape_matrix &&  //
              orientation == other.orientation &&    //
              type == other.type);
    };

    //! @brief Draw the region.
    void draw(const Color3ub& c, float scale = 1.f,
              const Point2f& offset = Point2f::Zero()) const;

    friend std::ostream& operator<<(std::ostream&, const OERegion&);

    friend std::istream& operator>>(std::istream&, OERegion&);

    //! @brief Center of the feature.
    Point2f coords;

    //! @brief Shape matrix encoding the ellipticity of the region.
    //!
    //! The shape matrix is the matrix $M$ that describes the ellipse
    //! $\varepsilon$, i.e.:
    //! $$ \varepsilon = \{ x \in R^2 : (x-c)^T M (x-c) = 1 \} $$
    //! where $c$ is the center of the region.
    Matrix2f shape_matrix;

    //! @brief Orientation of the region **after** shape normalization.
    //!
    //! This completely determines the affine transformation that transforms the
    //! unit circle to the elliptic shape of the region.
    float orientation;

    //! @{
    //! @brief Characterization of the feature type.
    float extremum_value;
    Type type;
    ExtremumType extremum_type;
    //! @}

  };

  //! @}

} /* namespace Sara */
} /* namespace DO */
