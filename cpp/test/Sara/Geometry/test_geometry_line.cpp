// ========================================================================== //
// This file is part of Sara, a basic set of libraries in C++ for computer
// vision.
//
// Copyright (C) 2020-present David Ok <david.ok8@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
// ========================================================================== //

#define BOOST_TEST_MODULE "Geometry/Tools/Projective"

#include <boost/test/unit_test.hpp>

#include <DO/Sara/Geometry/Algorithms/RobustEstimation/RANSAC.hpp>
#include <DO/Sara/Geometry/Algorithms/RobustEstimation/LineSolver.hpp>

#include "../AssertHelpers.hpp"


using namespace std;
using namespace DO::Sara;


BOOST_AUTO_TEST_CASE(test_line)
{
  const Eigen::Vector2d a = 2 * Eigen::Vector2d::UnitX();
  const Eigen::Vector2d b = Eigen::Vector2d::UnitY();

  const auto line_expected = Eigen::Vector3d{-0.5, -1, 1};
  const auto line = Projective::line(a.homogeneous().eval(),  //
                                     b.homogeneous().eval());

  BOOST_CHECK_SMALL(line.cross(line_expected).norm(), 1e-9);
}

BOOST_AUTO_TEST_CASE(test_random_samples)
{
  auto points = Tensor_<double, 2>{6, 2};
  points.matrix() <<                    0.00,                      0.00,
                                        1.00,                      1.10,
                                        3.40,                      3.46,
                                        9.80,                     10.10,
                     2 * std::cos(M_PI / 3.), -10 * std::sin(M_PI / 3.),
                         std::cos(M_PI / 6.),       std::sin(M_PI / 6.);

  const auto& num_points = points.size(0);

  auto index_samples = random_samples(10, 2, num_points);
  auto point_samples = to_coordinates(index_samples, points);

  SARA_DEBUG << "points =\n" << points.matrix() << std::endl;
  SARA_DEBUG << "points homogeneous =\n"
             << points.matrix().rowwise().homogeneous() << std::endl;
  SARA_DEBUG << "indices sampled =\n" << index_samples.matrix() << std::endl;
}

BOOST_AUTO_TEST_CASE(test_robust_line_fit)
{
  auto points = Tensor_<double, 2>{6, 3};
  points.matrix() <<                    0.00,                      0.00, 1,
                                        1.00,                      1.10, 1,
                                        3.40,                      3.46, 1,
                                        9.80,                     10.10, 1,
                     2 * std::cos(M_PI / 3.), -10 * std::sin(M_PI / 3.), 1,
                         std::cos(M_PI / 6.),       std::sin(M_PI / 6.), 1;

  auto line_solver = LineSolver2D<double>{};
  auto inlier_predicate = InlierPredicate<LinePointDistance2D<double>>{
      {},  //
      0.1  //
  };
  const auto& estimation = ransac(points,            //
                                 line_solver,       //
                                 inlier_predicate,  //
                                 10u);
  const auto& inliers = std::get<1>(estimation);
  BOOST_CHECK_EQUAL(inliers.flat_array().count(), 4);
}
