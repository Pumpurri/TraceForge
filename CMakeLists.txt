cmake_minimum_required(VERSION 3.20)

project(
    traceforge
    VERSION 0.1.0
    DESCRIPTION "Multi-sensor telemetry recording and deterministic replay"
    LANGUAGES CXX
)

option(TRACEFORGE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

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
    src/about.cpp
    src/telemetry/collector_service.cpp
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
endif()
