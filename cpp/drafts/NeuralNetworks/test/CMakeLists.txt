message(STATUS "  - DO_NeuralNetworks_Tests")

find_package(DO_Sara COMPONENTS Core REQUIRED)

find_package(TensorRT)
find_package(CUDA)

file(GLOB test_neuralnetworks_SOURCE_FILES FILES test_*.cpp)

file(GLOB test_tensorrt_SOURCE_FILES FILES test_*tensorrt*.cpp)
foreach (file ${test_tensorrt_SOURCE_FILES})
  list(REMOVE_ITEM test_neuralnetworks_SOURCE_FILES ${file})
endforeach()

foreach (file ${test_neuralnetworks_SOURCE_FILES})
  get_filename_component(filename "${file}" NAME_WE)
  sara_add_test(${filename} ${file} "${DO_Sara_LIBRARIES}" "DRAFTS/NeuralNetworks")
endforeach ()

if (CUDA_FOUND AND TensorRT_FOUND)
  foreach(file ${test_tensorrt_SOURCE_FILES})
    get_filename_component(filename ${file} NAME_WE)

    cuda_add_executable(${filename} ${file})

    target_include_directories(${filename}
      PRIVATE ${TensorRT_INCLUDE_DIRS})

    target_compile_definitions(${filename}
      PRIVATE -DBOOST_TEST_DYN_LINK)

    target_link_libraries(${filename}
      ${Boost_LIBRARIES}
      ${CUDA_LIBRARIES}
      ${TensorRT_LIBRARIES})

    set_target_properties(${filename}
      PROPERTIES
      COMPILE_FLAGS ${SARA_DEFINITIONS}
      RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

    add_test(NAME ${filename} COMMAND $<TARGET_FILE:${filename}>)
  endforeach()
endif ()