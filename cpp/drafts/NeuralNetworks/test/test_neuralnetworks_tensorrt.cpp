#include <DO/Sara/Core/DebugUtilities.hpp>
#include <DO/Sara/Core/StringFormat.hpp>
#include <DO/Sara/Core/Tensor.hpp>

#include <DO/Shakti/MultiArray.hpp>
#include <DO/Shakti/Utilities.hpp>

#include <drafts/NeuralNetworks/TensorRT/Helpers.hpp>

#include <termcolor/termcolor.hpp>

#include <fstream>
#include <sstream>


namespace sara = DO::Sara;
namespace shakti = DO::Shakti;


template <typename T, int N>
using PinnedTensor = sara::Tensor_<float, N, shakti::PinnedAllocator>;


auto engine_deleter(nvinfer1::ICudaEngine* engine) -> void
{
  if (engine != nullptr)
    engine->destroy();
  engine = nullptr;
}

auto runtime_deleter(nvinfer1::IRuntime* runtime) -> void
{
  if (runtime != nullptr)
    runtime->destroy();
  runtime = nullptr;
}

auto config_deleter(nvinfer1::IBuilderConfig* config) -> void
{
  if (config != nullptr)
    config->destroy();
  config = nullptr;
}

auto context_deleter(nvinfer1::IExecutionContext* context) -> void
{
  if (context != nullptr)
    context->destroy();
  context = nullptr;
};

using CudaEngineUniquePtr =
    std::unique_ptr<nvinfer1::ICudaEngine, decltype(&engine_deleter)>;
using RuntimeUniquePtr =
    std::unique_ptr<nvinfer1::IRuntime, decltype(&runtime_deleter)>;
using ConfigUniquePtr =
    std::unique_ptr<nvinfer1::IBuilderConfig, decltype(&config_deleter)>;
using ContextUniquePtr =
    std::unique_ptr<nvinfer1::IExecutionContext, decltype(&context_deleter)>;


template <typename NVInferObject>
auto delete_nvinfer_object(NVInferObject* object) -> void
{
  if (object != nullptr)
    object->destroy();
  object = nullptr;
}


auto load_from_model_weights(const std::string& trt_model_weights_filepath)
{
  auto runtime =
      std::unique_ptr<nvinfer1::IRuntime, decltype(&runtime_deleter)>{
          nvinfer1::createInferRuntime(DO::Sara::TensorRT::Logger::instance()),
          &runtime_deleter};

  auto model_weights_file = std::ifstream{trt_model_weights_filepath};
  if (!model_weights_file)
    throw std::runtime_error{"Failed to open model weights file!"};

  auto model_weights_stream = std::stringstream{};
  model_weights_stream << model_weights_file.rdbuf();

  // Count the number of bytes.
  model_weights_stream.seekg(0, std::ios::end);
  const auto model_weights_byte_size = model_weights_stream.tellg();

  // Rewind to the beginning of the file.
  model_weights_stream.seekg(0, std::ios::beg);

  // Read the file and transfer the data to the array of the bytes.
  auto model_weights = std::vector<char>(model_weights_byte_size);
  model_weights_stream.read(model_weights.data(), model_weights.size());

  // Deserialize the model weights data to initialize the CUDA inference engine.
  return CudaEngineUniquePtr{runtime->deserializeCudaEngine(
                                 model_weights.data(), model_weights.size()),
                             &engine_deleter};
}

auto save_model_weights(nvinfer1::ICudaEngine* engine,
                        const std::string& model_weights_filepath) -> void
{
  // Memory management.
  auto model_weights_deleter = [](nvinfer1::IHostMemory* model_stream) {
    if (model_stream != nullptr)
      model_stream->destroy();
    model_stream = nullptr;
  };

  // Serialize the model weights into the following data buffer.
  auto model_weights =
      std::unique_ptr<nvinfer1::IHostMemory, decltype(model_weights_deleter)>{
          engine->serialize(), model_weights_deleter};

  // Save in the disk.
  auto model_weights_stream = std::stringstream{};
  model_weights_stream.seekg(0, model_weights_stream.beg);
  model_weights_stream.write(static_cast<const char*>(model_weights->data()),
                             model_weights->size());

  auto model_weights_file = std::ofstream{model_weights_filepath};
  if (!model_weights_file)
    throw std::runtime_error{"Failed to create model weights file!"};
  model_weights_file << model_weights_stream.rdbuf();
}

auto main() -> int
{
  // List the available GPU devices.
  const auto devices = shakti::get_devices();
  for (const auto& device : devices)
    std::cout << device << std::endl;

  auto cuda_stream = sara::TensorRT::make_cuda_stream();

  auto builder = sara::TensorRT::make_builder();

  // Create a simple convolution operation.
  constexpr auto n = 1;
  constexpr auto h = 8;
  constexpr auto w = 8;
  constexpr auto kh = 3;
  constexpr auto kw = 3;
  constexpr auto ci = 1;
  constexpr auto co = 20;

  // Create artificial weights.
  const auto conv1_kernel_weights_vector =
      std::vector<float>(kh * kw * ci * co, 1.f);
  const auto conv1_bias_weights_vector = std::vector<float>(ci * co, 0.f);


  // Instantiate a network and automatically manager its memory.
  auto network = sara::TensorRT::make_network(builder.get());
  {
    SARA_DEBUG << termcolor::green << "Creating the network from scratch!"
               << std::endl;

    // Instantiate an input data.&
    auto image_tensor = network->addInput("image", nvinfer1::DataType::kFLOAT,
                                          nvinfer1::Dims3{n, h, w});

#ifdef CHECK_SCALE_OP
    const float scale = 3.f;
    const auto scale_weights =
        nvinfer1::Weights{nvinfer1::DataType::kFLOAT,             //
                          reinterpret_cast<const void*>(&scale),  //
                          1};

    auto scale_op = network->addScale(
        *image_tensor, nvinfer1::ScaleMode::kUNIFORM, {}, scale_weights, {});

    // Get the ouput tensor.
    auto image_scaled_tensor = scale_op->getOutput(0);
    network->markOutput(*image_scaled_tensor);
#else
    // Encapsulate the weights using TensorRT data structures.
    const auto conv1_kernel_weights = nvinfer1::Weights{
        nvinfer1::DataType::kFLOAT,
        reinterpret_cast<const void*>(conv1_kernel_weights_vector.data()),
        static_cast<std::int64_t>(conv1_kernel_weights_vector.size())};
    const auto conv1_bias_weights = nvinfer1::Weights{
        nvinfer1::DataType::kFLOAT,
        reinterpret_cast<const void*>(conv1_bias_weights_vector.data()),
        static_cast<std::int64_t>(conv1_bias_weights_vector.size())};

    // Create a convolutional function.
    const auto conv1_fn = network->addConvolution(
        *image_tensor,
        co,                    // number of filters
        {kh, kw},              // kernel sizes
        conv1_kernel_weights,  // convolution kernel weights
        conv1_bias_weights);   // bias weights

    // Get the ouput tensor.
    auto conv1 = conv1_fn->getOutput(0);
    network->markOutput(*conv1);
#endif
  }

  SARA_DEBUG << termcolor::green
             << "Setting the inference engine with the right configuration!"
             << termcolor::reset << std::endl;
  // Setup the engine.
  builder->setMaxBatchSize(1);

  // Create a inference configuration object.
  auto config =
      ConfigUniquePtr{builder->createBuilderConfig(), &config_deleter};
  config->setMaxWorkspaceSize(32);
  config->setFlag(nvinfer1::BuilderFlag::kGPU_FALLBACK);
  // If the GPU supports FP16 operations.
  // config->setFlag(nvinfer1::BuilderFlag::kFP16);

  // Create or load an engine.
  auto engine = CudaEngineUniquePtr{nullptr, &engine_deleter};
  {
    // Load inference engine from a model weights.
    constexpr auto load_engine_from_disk = false;
    if constexpr (load_engine_from_disk)
      engine = load_from_model_weights("");
    else
    {
      engine = CudaEngineUniquePtr{
          builder->buildEngineWithConfig(*network, *config),  //
          &engine_deleter};
      // save_model_weights("");
    }
  }

  // Perform a context to enqueue inference operations in C++.
  SARA_DEBUG << termcolor::green << "Setting the inference context!"
             << termcolor::reset << std::endl;
  auto context = ContextUniquePtr{engine->createExecutionContext(),  //
                                  &context_deleter};

  // Create som data and create two GPU device buffers.
  SARA_DEBUG << termcolor::red << "Creating input data and two device buffers!"
             << termcolor::reset << std::endl;
  auto image = PinnedTensor<float, 2>{h, w};
  image.matrix().fill(0.f);
  image(h / 2, w / 2) = 2.f;

  // Inspect the TensorRT log output: there is no padding!
  auto image_convolved =
      PinnedTensor<float, 3>{{co, h - (kh / 2) * 2, w - (kw / 2) * 2}};
  image_convolved.flat_array().fill(-1.f);

  // Fill some GPU buffers and perform inference.
  SARA_DEBUG << termcolor::green << "Perform inference on GPU!"
             << termcolor::reset << std::endl;

  auto device_buffers = std::vector{
      reinterpret_cast<void*>(image.data()),           //
      reinterpret_cast<void*>(image_convolved.data())  //
  };

  // Enqueue the CPU pinned <-> GPU tranfers and the convolution task.
  if (!context->enqueue(1, device_buffers.data(), *cuda_stream, nullptr))
  {
    SARA_DEBUG << termcolor::red << "Execution failed!" << termcolor::reset
               << std::endl;
  }

  // Wait for the completion of GPU operations.
  cudaStreamSynchronize(*cuda_stream);


  for (auto c = 0; c < co; ++c)
  {
    SARA_CHECK(c);
    std::cout << image_convolved[c].matrix() << std::endl;
  }

  return 0;
}
