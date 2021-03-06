// ========================================================================== //
// This file is part of DO-CV, a basic set of libraries in C++ for computer
// vision.
//
// Copyright (C) 2015 David Ok <david.ok8@gmail.com>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.
// ========================================================================== //

//! @example

#include <DO/Sara/Graphics.hpp>
#include <DO/Sara/ImageIO.hpp>
#include <DO/Sara/ImageProcessing.hpp>
#include <DO/Sara/VideoIO/VideoStream.hpp>

#include <DO/Shakti/ImageProcessing.hpp>
#include <DO/Shakti/MultiArray.hpp>
#include <DO/Shakti/Utilities/DeviceInfo.hpp>
#include <DO/Shakti/Utilities/Timer.hpp>

#include <gpufilter.h>


namespace sara = DO::Sara;
namespace shakti = DO::Shakti;


struct TicToc : sara::Timer
{
  static auto instance() -> TicToc&
  {
    static auto instance_ = TicToc();
    return instance_;
  }
};

void tic()
{
  TicToc::instance().restart();
}

void toc(const std::string& what)
{
  const auto elapsed = TicToc::instance().elapsed_ms();
  std::cout << "[" << what << "] " << elapsed <<  " ms" << std::endl;
}


GRAPHICS_MAIN()
{
  auto devices = shakti::get_devices();
  devices.front().make_current_device();
  std::cout << devices.front() << std::endl;

  constexpr auto use_low_resolution_video = false;
  constexpr auto video_filepath =
      use_low_resolution_video
          ?
          // Video sample with image sizes (320 x 240).
          src_path("Segmentation/orion_1.mpg")
          :
  // Video samples with image sizes (1920 x 1080).
#ifdef _WIN32
          "C:/Users/David/Desktop/david-archives/gopro-backup-2/GOPR0542.MP4"
#else
          "/home/david/Desktop/Datasets/sfm/Family.mp4"
#endif
      ;
  std::cout << video_filepath << std::endl;

  sara::VideoStream video_stream{video_filepath};
  auto video_frame_index = int{0};
  auto video_frame = video_stream.frame();

  auto in_frame = sara::Image<float>{video_stream.sizes()};

  // The pipeline is far from being optimized but despite that we almost get
  // real-time image processing.
  const auto sigma = 10.f;

  sara::create_window(video_frame.sizes());

  while (true)
  {
    tic();
    if (!video_stream.read())
      break;
    toc("CPU video decoding time");

    tic();
    {
      std::transform(video_frame.begin(), video_frame.end(), in_frame.begin(),
                     [](const sara::Rgb8& c) -> float {
                       auto gray = float{};
                       sara::smart_convert_color(c, gray);
                       return gray;
                     });
    }
    toc("CPU color conversion time");

    // I don't see crazy speed ups compared to the vanilla CUDA implementation.
    // Maybe the memory transfer from host CPU memory to CUDA GPU memory is
    // dominating...
    //
    // The recursive Gaussian implementation takes about 7ms to blur the image
    // just like the vanilla CUDA implementation.
    //
    // So a bit more investigation would be needed to understand why.
    shakti::tic();
    {
      gpufilter::gaussian_gpu(in_frame.data(), in_frame.width(),
                              in_frame.height(), sigma);
    }
    shakti::toc("Nehab's GPU gaussian filter");

    sara::display(in_frame);

    ++video_frame_index;
    std::cout << std::endl;
  }

  return 0;
}
