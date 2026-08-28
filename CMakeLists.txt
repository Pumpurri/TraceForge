cmake_minimum_required(VERSION 3.20)

project(
    traceforge
    VERSION 0.1.0
    DESCRIPTION "Multi-sensor telemetry recording and deterministic replay"
    LANGUAGES CXX
)

option(TRACEFORGE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(TRACEFORGE_ENABLE_ASAN "Enable AddressSanitizer" OFF)
option(TRACEFORGE_ENABLE_UBSAN "Enable UndefinedBehaviorSanitizer" OFF)
option(TRACEFORGE_ENABLE_TSAN "Enable ThreadSanitizer" OFF)
option(TRACEFORGE_BUILD_FUZZER "Build the libFuzzer recording target" OFF)

if(TRACEFORGE_ENABLE_ASAN AND TRACEFORGE_ENABLE_TSAN)
    message(FATAL_ERROR "AddressSanitizer and ThreadSanitizer cannot be combined")
endif()

set(TRACEFORGE_SANITIZERS "")
if(TRACEFORGE_ENABLE_ASAN)
    list(APPEND TRACEFORGE_SANITIZERS address)
endif()
if(TRACEFORGE_ENABLE_UBSAN)
    list(APPEND TRACEFORGE_SANITIZERS undefined)
endif()
if(TRACEFORGE_ENABLE_TSAN)
    list(APPEND TRACEFORGE_SANITIZERS thread)
endif()

if(TRACEFORGE_SANITIZERS)
    if(MSVC)
        message(FATAL_ERROR "TraceForge sanitizer options require Clang or GCC")
    endif()
    list(JOIN TRACEFORGE_SANITIZERS "," TRACEFORGE_SANITIZER_FLAGS)
endif()

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

function(traceforge_enable_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
        if(TRACEFORGE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()
    else()
        target_compile_options(
            ${target}
            PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wshadow
        )
        if(TRACEFORGE_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
    if(TRACEFORGE_SANITIZERS)
        target_compile_options(
            ${target}
            PRIVATE
                -fno-omit-frame-pointer
                "-fsanitize=${TRACEFORGE_SANITIZER_FLAGS}"
        )
        target_link_options(
            ${target}
            PRIVATE "-fsanitize=${TRACEFORGE_SANITIZER_FLAGS}"
        )
    endif()
endfunction()

configure_file(
    cmake/version.hpp.in
    generated/traceforge/version.hpp
    @ONLY
)

set(TRACEFORGE_PROTO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(TRACEFORGE_PROTO_FILE
    "${TRACEFORGE_PROTO_ROOT}/traceforge/v1/telemetry.proto"
)
set(TRACEFORGE_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(TRACEFORGE_GENERATED_PROTO_DIR
    "${TRACEFORGE_GENERATED_DIR}/traceforge/v1"
)
set(TRACEFORGE_PROTO_SOURCES
    "${TRACEFORGE_GENERATED_PROTO_DIR}/telemetry.pb.cc"
    "${TRACEFORGE_GENERATED_PROTO_DIR}/telemetry.grpc.pb.cc"
)
set(TRACEFORGE_PROTO_HEADERS
    "${TRACEFORGE_GENERATED_PROTO_DIR}/telemetry.pb.h"
    "${TRACEFORGE_GENERATED_PROTO_DIR}/telemetry.grpc.pb.h"
)

add_custom_command(
    OUTPUT ${TRACEFORGE_PROTO_SOURCES} ${TRACEFORGE_PROTO_HEADERS}
    COMMAND
        ${CMAKE_COMMAND} -E make_directory
        "${TRACEFORGE_GENERATED_PROTO_DIR}"
    COMMAND
        protobuf::protoc
        --proto_path=${TRACEFORGE_PROTO_ROOT}
        --cpp_out=${TRACEFORGE_GENERATED_DIR}
        --grpc_out=${TRACEFORGE_GENERATED_DIR}
        --plugin=protoc-gen-grpc=$<TARGET_FILE:gRPC::grpc_cpp_plugin>
        ${TRACEFORGE_PROTO_FILE}
    DEPENDS "${TRACEFORGE_PROTO_FILE}"
    VERBATIM
)

add_library(
    traceforge_proto
    ${TRACEFORGE_PROTO_SOURCES}
    ${TRACEFORGE_PROTO_HEADERS}
)
add_library(traceforge::proto ALIAS traceforge_proto)
target_compile_features(traceforge_proto PUBLIC cxx_std_20)
target_include_directories(
    traceforge_proto
    PUBLIC "$<BUILD_INTERFACE:${TRACEFORGE_GENERATED_DIR}>"
)
target_link_libraries(
    traceforge_proto
    PUBLIC
        protobuf::libprotobuf
        gRPC::grpc++
)

add_library(
    traceforge_core
    src/analysis/analyzer.cpp
    src/about.cpp
    src/generator/workload_generator.cpp
    src/generator/workload_publisher.cpp
    src/recording/log.cpp
    src/recording/log_consumer.cpp
    src/replay/replay.cpp
    src/telemetry/collector_service.cpp
    src/telemetry/queued_sink.cpp
    src/telemetry/validation.cpp
)
add_library(traceforge::core ALIAS traceforge_core)

target_compile_features(traceforge_core PUBLIC cxx_std_20)
target_include_directories(
    traceforge_core
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>"
)
target_link_libraries(traceforge_core PUBLIC traceforge::proto)
traceforge_enable_warnings(traceforge_core)

add_executable(traceforge app/main.cpp)
target_link_libraries(traceforge PRIVATE traceforge::core)
traceforge_enable_warnings(traceforge)

add_executable(
    traceforge_benchmark
    benchmarks/pipeline_benchmark.cpp
)
target_link_libraries(traceforge_benchmark PRIVATE traceforge::core)
traceforge_enable_warnings(traceforge_benchmark)

if(TRACEFORGE_BUILD_FUZZER)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "The recording fuzzer requires a Clang toolchain")
    endif()
    if(TRACEFORGE_ENABLE_TSAN)
        message(FATAL_ERROR "The recording fuzzer cannot be combined with TSan")
    endif()
    add_executable(
        traceforge_recording_fuzz
        fuzz/recording_parser_fuzz.cpp
    )
    target_link_libraries(traceforge_recording_fuzz PRIVATE traceforge::core)
    target_compile_options(traceforge_recording_fuzz PRIVATE -fsanitize=fuzzer)
    target_link_options(traceforge_recording_fuzz PRIVATE -fsanitize=fuzzer)
    traceforge_enable_warnings(traceforge_recording_fuzz)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(traceforge_smoke_test tests/smoke_test.cpp)
    target_link_libraries(traceforge_smoke_test PRIVATE traceforge::core)
    traceforge_enable_warnings(traceforge_smoke_test)

    add_test(NAME traceforge.smoke COMMAND traceforge_smoke_test)

    add_executable(
        traceforge_telemetry_validation_test
        tests/telemetry_validation_test.cpp
    )
    target_link_libraries(
        traceforge_telemetry_validation_test
        PRIVATE traceforge::core
    )
    traceforge_enable_warnings(traceforge_telemetry_validation_test)
    add_test(
        NAME traceforge.telemetry_validation
        COMMAND traceforge_telemetry_validation_test
    )

    add_executable(
        traceforge_telemetry_proto_test
        tests/telemetry_proto_test.cpp
    )
    target_link_libraries(
        traceforge_telemetry_proto_test
        PRIVATE traceforge::core
    )
    traceforge_enable_warnings(traceforge_telemetry_proto_test)
    add_test(
        NAME traceforge.telemetry_proto_roundtrip
        COMMAND traceforge_telemetry_proto_test
    )

    add_executable(
        traceforge_workload_generator_test
        tests/workload_generator_test.cpp
    )
    target_link_libraries(
        traceforge_workload_generator_test
        PRIVATE traceforge::core
    )
    traceforge_enable_warnings(traceforge_workload_generator_test)
    add_test(
        NAME traceforge.workload_generator
        COMMAND traceforge_workload_generator_test
    )

    add_executable(
        traceforge_bounded_queue_test
        tests/bounded_queue_test.cpp
    )
    target_link_libraries(
        traceforge_bounded_queue_test
        PRIVATE traceforge::core
    )
    traceforge_enable_warnings(traceforge_bounded_queue_test)
    add_test(
        NAME traceforge.bounded_queue
        COMMAND traceforge_bounded_queue_test
    )

    add_executable(
        traceforge_queued_sink_test
        tests/queued_sink_test.cpp
    )
    target_link_libraries(
        traceforge_queued_sink_test
        PRIVATE traceforge::core
    )
    traceforge_enable_warnings(traceforge_queued_sink_test)
    add_test(
        NAME traceforge.queued_sink
        COMMAND traceforge_queued_sink_test
    )

    add_executable(
        traceforge_workload_publisher_integration_test
        tests/workload_publisher_integration_test.cpp
    )
    target_link_libraries(
        traceforge_workload_publisher_integration_test
        PRIVATE traceforge::core
    )
    traceforge_enable_warnings(
        traceforge_workload_publisher_integration_test
    )
    add_test(
        NAME traceforge.workload_publisher_integration
        COMMAND traceforge_workload_publisher_integration_test
    )

    add_executable(
        traceforge_recording_log_test
        tests/recording_log_test.cpp
    )
    target_link_libraries(
        traceforge_recording_log_test
        PRIVATE traceforge::core
    )
    traceforge_enable_warnings(traceforge_recording_log_test)
    add_test(
        NAME traceforge.recording_log
        COMMAND traceforge_recording_log_test
    )

    add_executable(
        traceforge_replay_test
        tests/replay_test.cpp
    )
    target_link_libraries(
        traceforge_replay_test
        PRIVATE traceforge::core
    )
    traceforge_enable_warnings(traceforge_replay_test)
    add_test(
        NAME traceforge.replay
        COMMAND traceforge_replay_test
    )

    add_executable(
        traceforge_analysis_test
        tests/analysis_test.cpp
    )
    target_link_libraries(traceforge_analysis_test PRIVATE traceforge::core)
    traceforge_enable_warnings(traceforge_analysis_test)
    add_test(
        NAME traceforge.analysis
        COMMAND traceforge_analysis_test
    )

    add_executable(
        traceforge_collector_integration_test
        tests/collector_integration_test.cpp
    )
    target_link_libraries(
        traceforge_collector_integration_test
        PRIVATE traceforge::core
    )
    traceforge_enable_warnings(traceforge_collector_integration_test)
    add_test(
        NAME traceforge.collector_integration
        COMMAND traceforge_collector_integration_test
    )

    if(UNIX)
        find_program(TRACEFORGE_BASH_EXECUTABLE bash REQUIRED)
        add_test(
            NAME traceforge.graceful_shutdown
            COMMAND
                "${TRACEFORGE_BASH_EXECUTABLE}"
                "${CMAKE_CURRENT_SOURCE_DIR}/tests/graceful_shutdown_test.sh"
                "$<TARGET_FILE:traceforge>"
        )
        set_tests_properties(
            traceforge.graceful_shutdown
            PROPERTIES TIMEOUT 10
        )
    endif()
endif()
