// ========================================================================== //
// This file is part of Sara, a basic set of libraries in C++ for computer
// vision.
//
// Copyright (C) 2014-2016 David Ok <david.ok8@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
// ========================================================================== //

#define BOOST_TEST_MODULE "Core/Image/Crop"

#include <boost/test/unit_test.hpp>

#include <DO/Sara/Core/Image/Operations.hpp>


using namespace std;
using namespace DO::Sara;


class TestFixtureForImageCrop
{
protected:
  Image<float> image;
  Vector2i sizes;

public:
  TestFixtureForImageCrop()
  {
    sizes << 3, 4;
    image.resize(sizes);
    image.matrix() <<
      1, 1, 1,
      2, 2, 2,
      3, 3, 3,
      4, 4, 4;
  }
};

BOOST_FIXTURE_TEST_SUITE(TestImageCrop, TestFixtureForImageCrop)

BOOST_AUTO_TEST_CASE(test_safe_crop_image_within_bounds)
{
  const auto a = Vector2i{1, 1};
  const auto b = Vector2i{3, 2};
  auto safe_cropped_image = safe_crop(image, a, b);
  auto cropped_image = crop(image, a, b);

  auto true_cropped_image = Image<float, 2>{2, 1};
  true_cropped_image.matrix() << 2, 2;
  BOOST_CHECK_EQUAL(true_cropped_image.sizes(), safe_cropped_image.sizes());
  BOOST_CHECK_EQUAL(true_cropped_image.matrix(), safe_cropped_image.matrix());
  BOOST_CHECK_EQUAL(true_cropped_image.sizes(), cropped_image.sizes());
  BOOST_CHECK_EQUAL(true_cropped_image.matrix(), cropped_image.matrix());

  const auto x = 1, y = 1;
  const auto w = 2, h = 1;
  safe_cropped_image = safe_crop(image, Point2i{x, y}, Point2i{x + w, y + h});
  cropped_image = crop(image, Point2i{x, y}, Point2i{x + w, y + h});
  BOOST_CHECK_EQUAL(true_cropped_image.sizes(), safe_cropped_image.sizes());
  BOOST_CHECK_EQUAL(true_cropped_image.matrix(), safe_cropped_image.matrix());
  BOOST_CHECK_EQUAL(true_cropped_image.sizes(), cropped_image.sizes());
  BOOST_CHECK_EQUAL(true_cropped_image.matrix(), cropped_image.matrix());
}

BOOST_AUTO_TEST_CASE(test_safe_crop_image_out_of_bounds_1)
{
  const auto a = Vector2i{-3, -3};
  const auto b = Vector2i{0, 0};
  auto cropped_image = safe_crop(image, a, b);

  auto true_cropped_image = Image<float, 2>{3, 3};
  true_cropped_image.matrix().fill(0);
  BOOST_CHECK_EQUAL(true_cropped_image.sizes(), cropped_image.sizes());
  BOOST_CHECK_EQUAL(true_cropped_image.matrix(), cropped_image.matrix());

  const auto x = -3, y = -3;
  const auto w =  3, h =  3;
  cropped_image = safe_crop(image, Point2i{x, y}, Point2i{x + w, y + h});
  BOOST_CHECK_EQUAL(true_cropped_image.sizes(), cropped_image.sizes());
  BOOST_CHECK_EQUAL(true_cropped_image.matrix(), cropped_image.matrix());

  const auto c = Point2i{-2, -2};
  const auto r = 1;
  cropped_image = safe_crop(image, c, r);
  BOOST_CHECK_EQUAL(true_cropped_image.sizes(), cropped_image.sizes());
  BOOST_CHECK_EQUAL(true_cropped_image.matrix(), cropped_image.matrix());

}

BOOST_AUTO_TEST_CASE(test_safe_crop_image_out_of_bounds_2)
{
  const auto a = Vector2i{-1, -1};
  const auto b = Vector2i{2, 3};
  auto cropped_image = safe_crop(image, a, b);

  auto true_cropped_image = Image<float, 2>{3, 4};
  true_cropped_image.matrix() <<
    0, 0, 0,
    0, 1, 1,
    0, 2, 2,
    0, 3, 3;
  BOOST_CHECK_EQUAL(true_cropped_image.sizes(), cropped_image.sizes());
  BOOST_CHECK_EQUAL(true_cropped_image.matrix(), cropped_image.matrix());

  const auto x = -1, y = -1;
  const auto w =  3, h =  4;
  cropped_image = safe_crop(image, Point2i{x, y}, Point2i{x + w, y + h});
  BOOST_CHECK_EQUAL(true_cropped_image.sizes(), cropped_image.sizes());
  BOOST_CHECK_EQUAL(true_cropped_image.matrix(), cropped_image.matrix());
}

BOOST_AUTO_TEST_CASE(test_safe_crop_image_out_of_bounds_3)
{
  const auto a = Vector2i{-1, -1};
  const auto b = Vector2i{4, 5};
  const auto cropped_image = safe_crop(image, a, b);
  auto true_cropped_image = Image<float, 2>{5, 6};
  true_cropped_image.matrix() <<
    0, 0, 0, 0, 0,
    0, 1, 1, 1, 0,
    0, 2, 2, 2, 0,
    0, 3, 3, 3, 0,
    0, 4, 4, 4, 0,
    0, 0, 0, 0, 0;
  BOOST_CHECK_EQUAL(true_cropped_image.sizes(), cropped_image.sizes());
  BOOST_CHECK_EQUAL(true_cropped_image.matrix(), cropped_image.matrix());
}


BOOST_AUTO_TEST_SUITE_END()
