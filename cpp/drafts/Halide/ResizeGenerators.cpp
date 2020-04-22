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

#include "MyHalide.hpp"


namespace {

  using namespace Halide;

  template <typename T>
  class Upscale : public Halide::Generator<Upscale<T>>
  {
  public:
    using Base = Generator<Upscale<T>>;
    using Base::get_target;

    template <typename T2>
    using Input = typename Base::template Input<T2>;

    template <typename T2>
    using Output = typename Base::template Output<T2>;


    GeneratorParam<int> tile_x{"tile_x", 32};
    GeneratorParam<int> tile_y{"tile_y", 32};

    Input<Buffer<T>> input{"input", 4};
    Output<Buffer<T>> output{"output", 4};

    Var x{"x"}, y{"y"}, c{"c"}, n{"n"};
    Var xo{"xo"}, yo{"yo"}, co{"co"}, no{"no"};
    Var xi{"xi"}, yi{"yi"}, ci{"ci"}, ni{"ni"};

    void generate()
    {
      Expr w_in = cast<float>(input.dim(0).extent());
      Expr h_in = cast<float>(input.dim(1).extent());
      Expr w_out = cast<float>(output.dim(0).extent());
      Expr h_out = cast<float>(output.dim(1).extent());

      Expr xx = x * w_in / w_out;
      Expr yy = y * h_in / h_out;

      auto input_padded = BoundaryConditions::repeat_edge(input);

      auto wx = xx - floor(xx);
      auto wy = yy - floor(yy);

      auto xr = cast<int>(xx);
      auto yr = cast<int>(yy);

      output(x, y, c, n) =
          (1 - wx) * (1 - wy) * input_padded(xr, yr, c, n) +  //
          wx * (1 - wy) * input_padded(xr + 1, yr, c, n) +    //
          (1 - wx) * wy * input_padded(xr, yr + 1, c, n) +    //
          wx * wy * input_padded(xr + 1, yr + 1, c, n);
    }

    void schedule()
    {
      input.dim(0).set_stride(Expr());  // Use an undefined Expr to
                                        // mean there is no
                                        // constraint.

      output.dim(0).set_stride(Expr());

      Expr input_is_planar = input.dim(0).stride() == 1;
      Expr input_is_interleaved = input.dim(0).stride() == 3 and  //
                                  input.dim(2).stride() == 1 and  //
                                  input.dim(2).extent() == 3;

      Expr output_is_planar = output.dim(0).stride() == 1;
      Expr output_is_interleaved = output.dim(0).stride() == 3 and  //
                                   output.dim(2).stride() == 1 and  //
                                   output.dim(2).extent() == 3;

      // GPU schedule.
      if (get_target().has_gpu_feature())
      {
        output.specialize(input_is_planar && output_is_planar)
            .gpu_tile(x, y, c, xo, yo, co, xi, yi, ci, tile_x, tile_y, 1,
                      TailStrategy::GuardWithIf);

        output.specialize(input_is_interleaved && output_is_interleaved)
            .gpu_tile(x, y, c, xo, yo, co, xi, yi, ci, tile_x, tile_y, 3,
                      TailStrategy::GuardWithIf);
      }

      // Hexagon schedule.
      else if (get_target().features_any_of({Target::HVX_64, Target::HVX_128}))
      {
        const auto vector_size =
            get_target().has_feature(Target::HVX_128) ? 128 : 64;

        output.specialize(input_is_planar && output_is_planar)
            .hexagon()
            .prefetch(input, y, 2)
            .split(y, yo, yi, 128)
            .parallel(yo)
            .vectorize(x, vector_size, TailStrategy::GuardWithIf);
      }

      // CPU schedule.
      else
      {
        output.specialize(input_is_planar && output_is_planar)
            .split(y, yo, yi, 8)
            .parallel(yo)
            .vectorize(x, 8, TailStrategy::GuardWithIf);

        output.specialize(input_is_interleaved && output_is_interleaved)
            .reorder(c, x, y)
            .unroll(c);
      }
    }
  };

  class Enlarge : public Halide::Generator<Enlarge>
  {
  public:
    GeneratorParam<int> tile_x{"tile_x", 8};
    GeneratorParam<int> tile_y{"tile_y", 8};

    Input<Buffer<float>> input{"input", 3};
    Input<int[2]> in_sizes{"in_sizes"};
    Input<int[2]> out_sizes{"out_sizes"};
    Output<Buffer<float>> output{"enlarged_input", 3};

    Var x{"x"}, y{"y"}, c{"c"};
    Var xo{"xo"}, yo{"yo"}, co{"co"};
    Var xi{"xi"}, yi{"yi"}, ci{"ci"};

    void generate()
    {
      Expr w_in = cast<float>(in_sizes[0]);
      Expr h_in = cast<float>(in_sizes[1]);
      Expr w_out = cast<float>(out_sizes[0]);
      Expr h_out = cast<float>(out_sizes[1]);

      Expr xx = x * w_in / w_out;
      Expr yy = y * h_in / h_out;

      auto input_padded = BoundaryConditions::repeat_edge(input);

      auto wx = xx - floor(xx);
      auto wy = yy - floor(yy);

      auto xr = cast<int>(xx);
      auto yr = cast<int>(yy);

      output(x, y, c) = (1 - wx) * (1 - wy) * input_padded(xr, yr, c) +  //
                        wx * (1 - wy) * input_padded(xr + 1, yr, c) +    //
                        (1 - wx) * wy * input_padded(xr, yr + 1, c) +    //
                        wx * wy * input_padded(xr + 1, yr + 1, c);
    }

    void schedule()
    {
      input.dim(0).set_stride(Expr());  // Use an undefined Expr to
                                        // mean there is no
                                        // constraint.

      output.dim(0).set_stride(Expr());

      Expr input_is_planar = input.dim(0).stride() == 1;
      Expr input_is_interleaved = input.dim(0).stride() == 3 and  //
                                  input.dim(2).stride() == 1 and  //
                                  input.dim(2).extent() == 3;

      Expr output_is_planar = output.dim(0).stride() == 1;
      Expr output_is_interleaved = output.dim(0).stride() == 3 and  //
                                   output.dim(2).stride() == 1 and  //
                                   output.dim(2).extent() == 3;

      // GPU schedule.
      if (get_target().has_gpu_feature())
      {
        output.specialize(input_is_planar && output_is_planar)
            .gpu_tile(x, y, c, xo, yo, co, xi, yi, ci, tile_x, tile_y, 1,
                      TailStrategy::GuardWithIf);

        output.specialize(input_is_interleaved && output_is_interleaved)
            .gpu_tile(x, y, c, xo, yo, co, xi, yi, ci, tile_x, tile_y, 3,
                      TailStrategy::GuardWithIf);
      }

      // Hexagon schedule.
      else if (get_target().features_any_of({Target::HVX_64, Target::HVX_128}))
      {
        const auto vector_size =
            get_target().has_feature(Target::HVX_128) ? 128 : 64;

        output.specialize(input_is_planar && output_is_planar)
            .hexagon()
            .prefetch(input, y, 2)
            .split(y, yo, yi, 128)
            .parallel(yo)
            .vectorize(x, vector_size, TailStrategy::GuardWithIf);
      }

      // CPU schedule.
      else
      {
        output.specialize(input_is_planar && output_is_planar)
            .split(y, yo, yi, 8)
            .parallel(yo)
            .vectorize(x, 8, TailStrategy::GuardWithIf);

        output.specialize(input_is_interleaved && output_is_interleaved)
            .reorder(c, x, y)
            .unroll(c);
      }
    }
  };

}  // namespace


HALIDE_REGISTER_GENERATOR(Upscale<float>, shakti_upscale_32f)
HALIDE_REGISTER_GENERATOR(Enlarge, shakti_enlarge)
