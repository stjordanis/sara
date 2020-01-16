#include <DO/Sara/Core.hpp>
#include <DO/Sara/Graphics.hpp>

#include "Halide.h"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <dlfcn.h>
#endif

#include <cstdio>


namespace sara = DO::Sara;

using namespace Halide;


Var x, y, c, i, ii, xo, yo, xi, yi;


auto find_gpu_target() -> Target
{
  // Start with a target suitable for the machine you're running this on.
  Target target = get_host_target();

  // Uncomment the following lines to try CUDA instead:
  target.set_feature(Target::CUDA);
  return target;

#ifdef _WIN32
  if (LoadLibraryA("d3d12.dll") != nullptr)
  {
    target.set_feature(Target::D3D12Compute);
  }
  else if (LoadLibraryA("OpenCL.dll") != nullptr)
  {
    target.set_feature(Target::OpenCL);
  }
#elif __APPLE__
  // OS X doesn't update its OpenCL drivers, so they tend to be broken.
  // CUDA would also be a fine choice on machines with NVidia GPUs.
  if (dlopen(
          "/System/Library/Frameworks/Metal.framework/Versions/Current/Metal",
          RTLD_LAZY) != NULL)
  {
    target.set_feature(Target::Metal);
  }
#else
  if (dlopen("libOpenCL.so", RTLD_LAZY) != NULL)
  {
    target.set_feature(Target::OpenCL);
  }
#endif

  return target;
}


class Pipeline
{
public:
  Func lut, padded, padded16, sharpen, curved;
  Buffer<uint8_t> input;


  Pipeline(Buffer<uint8_t> in)
    : input(in)
  {
    // Lookup table.
    lut(i) = cast<uint8_t>(clamp(pow(i / 255.f, 1.2f) * 255.0f, 0, 255));

    // Padded image.
    padded(x, y, c) = input(clamp(x, 0, input.width() - 1),
                            clamp(y, 0, input.height() - 1), c);


    padded16(x, y, c) = cast<uint16_t>(padded(x, y, c));
    sharpen(x, y, c) = padded16(x, y, c) * 2 -
                       (padded16(x - 1, y, c) + padded16(x, y - 1, c) +
                        padded16(x + 1, y, c) + padded16(x, y + 1, c)) /
                           4;

    curved(x, y, c) = lut(sharpen(x, y, c));
  }

  bool schedule_for_cpu()
  {
    lut.compute_root();

    curved.reorder(c, x, y).bound(c, 0, 3).unroll(c);

    Var yo, yi;
    curved.split(y, yo, yi, 16).parallel(yo);

    sharpen.compute_at(curved, yi);

    sharpen.vectorize(x, 16);

    padded.store_at(curved, yo).compute_at(curved, yi);

    padded.vectorize(x, 16);

    Target target = get_host_target();
    curved.compile_jit(target);

    return true;
  }

  bool schedule_for_gpu()
  {
    Target target = find_gpu_target();
    if (!target.has_gpu_feature())
    {
      return false;
    }

    lut.compute_root();

    Var block, thread;
    lut.split(i, block, thread, 16);
    lut.gpu_blocks(block).gpu_threads(thread);

    curved.reorder(c, x, y).bound(c, 0, 3).unroll(c);

    curved.gpu_tile(x, y, xo, yo, xi, yi, 8, 8);

    padded.compute_at(curved, xo);

    padded.gpu_threads(x, y);

    printf("Target: %s\n", target.to_string().c_str());
    curved.compile_jit(target);

    return true;
  }
};


GRAPHICS_MAIN()
{
  // Load an image.
  auto image = sara::Image<sara::Rgb8>{};
  if (!sara::load_from_dialog_box(image))
    return EXIT_FAILURE;


  // Show the image.
  sara::create_window(image.sizes());
  sara::display(image);
  sara::get_key();


  // Output image;
  auto image_altered = image;
  image.flat_array().fill(sara::Black8);

  // Start the processing.
  auto timer = sara::Timer{};
  timer.restart();
  {
    auto input =
        Halide::Buffer<uint8_t>{reinterpret_cast<uint8_t*>(image.data()),
                                {image.width(), image.height(), 3}};

    // printf("Running pipeline on CPU:\n");
    // ::Pipeline p1(input);
    // p1.schedule_for_cpu();

    printf("Running pipeline on GPU:\n");
    ::Pipeline p2(input);
    const auto has_gpu_target = p2.schedule_for_gpu();
    if (!has_gpu_target)
      printf("No GPU target available on the host\n");
    // p2.schedule_for_cpu();

    auto output = Halide::Buffer<uint8_t>{
        reinterpret_cast<uint8_t*>(image_altered.data()),
        {image_altered.width(), image_altered.height(), 3}};

    p2.curved.realize(output);
  }
  const auto elapsed = timer.elapsed_ms();
  std::cout << "Computation time = " << elapsed << " ms" << std::endl;

  // Show the result.
  sara::display(image_altered);
  sara::get_key();

  sara::close_window();


  return 0;
}
