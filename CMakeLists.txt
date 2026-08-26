cmake_minimum_required(VERSION 3.20)

project(
    traceforge
    VERSION 0.1.0
    DESCRIPTION "Multi-sensor telemetry recording and deterministic replay"
    LANGUAGES CXX
)

option(TRACEFORGE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

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

add_library(traceforge_core src/about.cpp)
add_library(traceforge::core ALIAS traceforge_core)

target_compile_features(traceforge_core PUBLIC cxx_std_20)
target_include_directories(
    traceforge_core
    PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/generated>"
)
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
endif()
