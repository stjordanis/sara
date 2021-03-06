#include "nvidia-video-codec-sdk-9.1.23/NvCodec/NvDecoder/NvDecoder.h"
#include "nvidia-video-codec-sdk-9.1.23/Utils/ColorSpace.h"
#include "nvidia-video-codec-sdk-9.1.23/Utils/FFmpegDemuxer.h"
#include "nvidia-video-codec-sdk-9.1.23/Utils/NvCodecUtils.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <cuda.h>
#include <cudaGL.h>

#include <DO/Sara/Core/DebugUtilities.hpp>


simplelogger::Logger* logger =
    simplelogger::LoggerFactory::CreateConsoleLogger();

namespace driver_api {

  auto init()
  {
    ck(cuInit(0));
  }

  auto get_device_count()
  {
    auto num_gpus = 0;
    ck(cuDeviceGetCount(&num_gpus));
    return num_gpus;
  }

  struct CudaContext
  {
    CUcontext cuda_context{NULL};
    CUdevice cuda_device{NULL};
    int gpu_id{-1};

    CudaContext(int gpu_id_ = 0)
      : gpu_id{gpu_id_}
    {
      ck(cuDeviceGet(&cuda_device, gpu_id));

      char device_name[80];
      ck(cuDeviceGetName(device_name, sizeof(device_name), cuda_device));
      std::cout << "GPU in use: " << device_name << std::endl;

      ck(cuCtxCreate(&cuda_context, CU_CTX_BLOCKING_SYNC, cuda_device));
    }

    CudaContext(CudaContext&& other)
    {
      std::swap(gpu_id, other.gpu_id);
      std::swap(cuda_context, other.cuda_context);
      std::swap(cuda_device, other.cuda_device);
    }

    CudaContext(const CudaContext&) = delete;

    ~CudaContext()
    {
      if (cuda_context)
      {
        ck(cuCtxDestroy(cuda_context));
        cuda_context = NULL;
        cuda_device = NULL;
        gpu_id = -1;
      }
    }

    operator CUcontext() const
    {
      return cuda_context;
    }

    auto make_current()
    {
      ck(cuCtxSetCurrent(cuda_context));
    }
  };

  struct DeviceBuffer
  {
    int width{};
    int height{};
    CUdeviceptr data{0};

    DeviceBuffer(int width_, int height_)
      : width{width_}
      , height{height_}
    {
      ck(cuMemAlloc(&data, width * height * 4));
      ck(cuMemsetD8(data, 0, width * height * 4));
    }

    ~DeviceBuffer()
    {
      if (data)
        ck(cuMemFree(data));
    }
  };

}  // namespace driver_api


struct VideoStream
{
  mutable FFmpegDemuxer demuxer;
  NvDecoder decoder;

  std::uint8_t** raw_frame_packet{nullptr};

  std::int32_t num_frames_decoded{};
  std::int32_t frame_index{};

  struct EncodedVideoBuffer
  {
    std::uint8_t* data{nullptr};
    std::int32_t size{};
  } frame_data_compressed;

  struct DeviceFrameBuffer
  {
    std::uint8_t* cuda_device_ptr{nullptr};
    std::int32_t pitch_size{};
  } device_frame_buffer;

  VideoStream(const std::string& video_filepath,
              const driver_api::CudaContext& context)
    : demuxer{video_filepath.c_str()}
    , decoder{context.cuda_context, true,
              FFmpeg2NvCodecId(demuxer.GetVideoCodec())}
  {
  }

  auto width() const
  {
    return demuxer.GetWidth();
  }

  auto height() const
  {
    return demuxer.GetHeight();
  }

  auto decode() -> void
  {
    // Initialize the video stream.
    do
    {
      demuxer.Demux(&frame_data_compressed.data, &frame_data_compressed.size);
      decoder.Decode(frame_data_compressed.data, frame_data_compressed.size,
                     &raw_frame_packet, &num_frames_decoded);
    } while (num_frames_decoded <= 0);
  }

  auto read_decoded_frame_packet(driver_api::DeviceBuffer& rgba_frame_buffer)
      -> void
  {
    // Launch CUDA kernels for colorspace conversion from raw video to raw
    // image formats which OpenGL textures can work with
    if (decoder.GetBitDepth() == 8)
    {
      if (decoder.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444)
        YUV444ToColor32<BGRA32>(
            (uint8_t*) raw_frame_packet[frame_index], decoder.GetWidth(),
            (uint8_t*) rgba_frame_buffer.data, rgba_frame_buffer.width * 4,
            decoder.GetWidth(), decoder.GetHeight());
      else  // default assumed NV12
        Nv12ToColor32<BGRA32>(
            (uint8_t*) raw_frame_packet[frame_index], decoder.GetWidth(),
            (uint8_t*) rgba_frame_buffer.data, rgba_frame_buffer.width * 4,
            decoder.GetWidth(), decoder.GetHeight());
    }
    else
    {
      if (decoder.GetOutputFormat() == cudaVideoSurfaceFormat_YUV444)
        YUV444P16ToColor32<BGRA32>(
            (uint8_t*) raw_frame_packet[frame_index], 2 * decoder.GetWidth(),
            (uint8_t*) rgba_frame_buffer.data, rgba_frame_buffer.width * 4,
            decoder.GetWidth(), decoder.GetHeight());
      else  // default assumed P016
        P016ToColor32<BGRA32>(
            (uint8_t*) raw_frame_packet[frame_index], 2 * decoder.GetWidth(),
            (uint8_t*) rgba_frame_buffer.data, rgba_frame_buffer.width * 4,
            decoder.GetWidth(), decoder.GetHeight());
    }

    ++frame_index;

    if (frame_index == num_frames_decoded)
      num_frames_decoded = frame_index = 0;
  }

  auto read(driver_api::DeviceBuffer& rgba_frame_buffer)
  {
    if (num_frames_decoded == 0)
      decode();

    LOG(INFO) << decoder.GetVideoInfo();

    if (frame_index < num_frames_decoded)
      read_decoded_frame_packet(rgba_frame_buffer);
  }
};


//! @brief OpenGL stuff.
//! @{
struct PixelBuffer
{
  GLuint _pbo;

  auto allocate(int w, int h) -> void
  {
    glGenBuffersARB(1, &_pbo);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, _pbo);
    glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, w * h * 4, NULL,
                    GL_STREAM_DRAW_ARB);
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);
  }

  auto bind() const -> void
  {
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, _pbo);
  }

  auto release() -> void
  {
    glDeleteBuffersARB(1, &_pbo);
  }
};

struct Texture
{
  int width;
  int height;
  GLuint _tex;

  auto allocate(int w, int h) -> void
  {
    width = w;
    height = h;

    glGenTextures(1, &_tex);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _tex);
    glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA8, w, h, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);
    glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
  }

  auto release() -> void
  {
    glDeleteTextures(1, &_tex);
    width = 0;
    height = 0;
  }

  auto bind() const
  {
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, _tex);
  }

  auto display() -> void
  {
    glBegin(GL_QUADS);
    glTexCoord2f(0, static_cast<GLfloat>(height));
    glVertex2f(0, 0);
    glTexCoord2f(static_cast<GLfloat>(width), static_cast<GLfloat>(height));
    glVertex2f(1, 0);
    glTexCoord2f(static_cast<GLfloat>(width), 0);
    glVertex2f(1, 1);
    glTexCoord2f(0, 0);
    glVertex2f(0, 1);
    glEnd();
    glBindTexture(GL_TEXTURE_RECTANGLE_ARB, 0);
  }
};

struct Shader
{
  GLuint _shader;

  auto initialize() -> void
  {
    static const char* code =
        "!!ARBfp1.0\n"
        "TEX result.color, fragment.texcoord, texture[0], RECT; \n"
        "END";
    glGenProgramsARB(1, &_shader);
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, _shader);
    glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
                       (GLsizei) strlen(code), (GLubyte*) code);
  }

  auto enable() -> void
  {
    glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, _shader);
    glEnable(GL_FRAGMENT_PROGRAM_ARB);
  }

  auto disable()
  {
    glDisable(GL_FRAGMENT_PROGRAM_ARB);
  }

  auto release() -> void
  {
    glDeleteProgramsARB(1, &_shader);
  }
};
//! @}


//! @brief CUDA/OpenGL interoperability.
//! @{
struct CudaGraphicsResource
{
  CUgraphicsResource cuda_resource;
  CUdeviceptr device_data_ptr;
  size_t device_data_size;

  CudaGraphicsResource(const PixelBuffer& pbo)
  {
    ck(cuGraphicsGLRegisterBuffer(&cuda_resource, pbo._pbo,
                                  CU_GRAPHICS_REGISTER_FLAGS_WRITE_DISCARD));
    ck(cuGraphicsMapResources(1, &cuda_resource, 0));
    ck(cuGraphicsResourceGetMappedPointer(&device_data_ptr, &device_data_size,
                                          cuda_resource));
  }

  ~CudaGraphicsResource()
  {
    ck(cuGraphicsUnmapResources(1, &cuda_resource, 0));
    ck(cuGraphicsUnregisterResource(cuda_resource));
  }
};

auto cuda_async_copy(const driver_api::DeviceBuffer& src, PixelBuffer& dst)
    -> void
{
  CUDA_MEMCPY2D m{};
  m.srcMemoryType = CU_MEMORYTYPE_DEVICE;

  const auto cuda_resource = CudaGraphicsResource{dst};

  // Source is dpFrame into which Decode() function writes data of individual
  // frame present in BGRA32 format.
  m.srcDevice = src.data;
  m.srcPitch = src.width * 4;

  // Destination is OpenGL buffer object mapped as a CUDA resource.
  m.dstMemoryType = CU_MEMORYTYPE_DEVICE;
  m.dstDevice = cuda_resource.device_data_ptr;
  m.dstPitch = cuda_resource.device_data_size / src.height;
  m.WidthInBytes = src.width * 4;
  m.Height = src.height;

  // Asynchronous copy from 2D device buffer to OpenGL buffer.
  ck(cuMemcpy2DAsync(&m, 0));
}
//! @}

inline void get_output_format_names(unsigned short output_format_mask,
                                    char* OutputFormats)
{
  if (output_format_mask == 0)
  {
    strcpy(OutputFormats, "N/A");
    return;
  }

  if (output_format_mask & (1U << cudaVideoSurfaceFormat_NV12))
    strcat(OutputFormats, "NV12 ");

  if (output_format_mask & (1U << cudaVideoSurfaceFormat_P016))
    strcat(OutputFormats, "P016 ");

  if (output_format_mask & (1U << cudaVideoSurfaceFormat_YUV444))
    strcat(OutputFormats, "YUV444 ");

  if (output_format_mask & (1U << cudaVideoSurfaceFormat_YUV444_16Bit))
    strcat(OutputFormats, "YUV444P16 ");

  return;
}

inline void show_decoder_capability()
{
  ck(cuInit(0));
  int num_gpus = 0;
  ck(cuDeviceGetCount(&num_gpus));
  std::cout << "Decoder Capability" << std::endl << std::endl;
  const char* codec_names[] = {
      "JPEG", "MPEG1", "MPEG2", "MPEG4", "H264", "HEVC", "HEVC", "HEVC",
      "HEVC", "HEVC",  "HEVC",  "VC1",   "VP8",  "VP9",  "VP9",  "VP9"};
  const char* chroma_format_strings[] = {"4:0:0", "4:2:0", "4:2:2", "4:4:4"};
  char output_formats[64];
  cudaVideoCodec codecs[] = {
      cudaVideoCodec_JPEG,  cudaVideoCodec_MPEG1, cudaVideoCodec_MPEG2,
      cudaVideoCodec_MPEG4, cudaVideoCodec_H264,  cudaVideoCodec_HEVC,
      cudaVideoCodec_HEVC,  cudaVideoCodec_HEVC,  cudaVideoCodec_HEVC,
      cudaVideoCodec_HEVC,  cudaVideoCodec_HEVC,  cudaVideoCodec_VC1,
      cudaVideoCodec_VP8,   cudaVideoCodec_VP9,   cudaVideoCodec_VP9,
      cudaVideoCodec_VP9};
  int bit_depth_minus_8[] = {0, 0, 0, 0, 0, 0, 2, 4, 0, 2, 4, 0, 0, 0, 2, 4};

  cudaVideoChromaFormat chroma_formats[] = {
      cudaVideoChromaFormat_420, cudaVideoChromaFormat_420,
      cudaVideoChromaFormat_420, cudaVideoChromaFormat_420,
      cudaVideoChromaFormat_420, cudaVideoChromaFormat_420,
      cudaVideoChromaFormat_420, cudaVideoChromaFormat_420,
      cudaVideoChromaFormat_444, cudaVideoChromaFormat_444,
      cudaVideoChromaFormat_444, cudaVideoChromaFormat_420,
      cudaVideoChromaFormat_420, cudaVideoChromaFormat_420,
      cudaVideoChromaFormat_420, cudaVideoChromaFormat_420};

  for (int gpu_id = 0; gpu_id < num_gpus; ++gpu_id)
  {
    auto cuda_context = driver_api::CudaContext{gpu_id};

    for (auto i = 0u; i < sizeof(codecs) / sizeof(codecs[0]); ++i)
    {
      CUVIDDECODECAPS decode_caps = {};
      decode_caps.eCodecType = codecs[i];
      decode_caps.eChromaFormat = chroma_formats[i];
      decode_caps.nBitDepthMinus8 = bit_depth_minus_8[i];

      cuvidGetDecoderCaps(&decode_caps);

      output_formats[0] = '\0';
      get_output_format_names(decode_caps.nOutputFormatMask, output_formats);

      // setw() width = maximum_width_of_string + 2 spaces
      std::cout << "Codec  " << std::left << std::setw(7) << codec_names[i]
                << "BitDepth  " << std::setw(4)
                << decode_caps.nBitDepthMinus8 + 8 << "ChromaFormat  "
                << std::setw(7)
                << chroma_format_strings[decode_caps.eChromaFormat]
                << "Supported  " << std::setw(3)
                << (int) decode_caps.bIsSupported << "MaxWidth  "
                << std::setw(7) << decode_caps.nMaxWidth << "MaxHeight  "
                << std::setw(7) << decode_caps.nMaxHeight << "MaxMBCount  "
                << std::setw(10) << decode_caps.nMaxMBCount << "MinWidth  "
                << std::setw(5) << decode_caps.nMinWidth << "MinHeight  "
                << std::setw(5) << decode_caps.nMinHeight << "SurfaceFormat  "
                << std::setw(11) << output_formats << std::endl;
    }

    std::cout << std::endl;
  }
}

int main()
{
  // Initialize CUDA driver.
  driver_api::init();

  // Create a CUDA context so that we can use the GPU device.
  const auto gpu_id = 0;
  auto cuda_context = driver_api::CudaContext{gpu_id};
  cuda_context.make_current();

  using namespace std::string_literals;
#ifdef _WIN32
  const auto video_filepath =
      "C:/Users/David/Desktop/david-archives/gopro-backup-2/GOPR0542.MP4"s;
#elif __APPLE__
  const auto video_filepath =
      "/Users/david/Desktop/Datasets/humanising-autonomy/turn_bikes.mp4"s;
#else
  // const auto video_filepath = "/home/david/Desktop/test.mp4"s;
  const auto video_filepath = "/home/david/Desktop/Datasets/sfm/Family.mp4"s;
  //const auto video_filepath = "/home/david/Desktop/GOPR0542.MP4"s;
#endif

  // Initialize a CUDA-powered video streamer object.
  VideoStream video_stream{video_filepath, cuda_context};

  // Create a video frame buffer.
  driver_api::DeviceBuffer device_rgba_buffer{video_stream.width(),
                                              video_stream.height()};

  // Initialize GLFW.
  if (!glfwInit())
    throw std::runtime_error{"Failed to initialize GLFW!"};

  // Open a window on which GLFW can operate.
  const auto w = video_stream.width();
  const auto h = video_stream.height();
  auto window = glfwCreateWindow(w, h, "Hello World", nullptr, nullptr);
  if (window == nullptr)
    throw std::runtime_error{"Failed to create GLFW window!"};
  glfwMakeContextCurrent(window);

  // Initialize GLEW.
  glewInit();

  // Initialize OpenGL resources.
  auto pbo = PixelBuffer{};
  auto texture = Texture{};
  auto shader = Shader{};
  pbo.allocate(w, h);
  texture.allocate(w, h);
  shader.initialize();

  // Setup OpenGL settings.
  glDisable(GL_DEPTH_TEST);

  // Setup the modelview transformations and the viewport.
  glViewport(0, 0, w, h);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, 1.0, 0.0, 1.0, 0.0, 1.0);


  // Display stuff.
  while (!glfwWindowShouldClose(window))
  {
    // Read the decoded frame and store it in a CUDA device buffer.
    video_stream.read(device_rgba_buffer);

    // Copy the device buffer data to the pixel buffer object.
    cuda_async_copy(device_rgba_buffer, pbo);

    // Upload the pixel buffer object data to the texture object.
    pbo.bind();
    texture.bind();
    glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, 0, 0, texture.width,
                    texture.height, GL_BGRA, GL_UNSIGNED_BYTE, 0);
    // Unbind PBO.
    glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, 0);

    shader.enable();
    texture.display();
    shader.disable();

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  pbo.release();
  texture.release();
  shader.release();

  return 0;
}
