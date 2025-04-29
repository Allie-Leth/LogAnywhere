cmake_minimum_required(VERSION 3.14)
project(BufferUtilsTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --------------------------------------------------
# Coverage option
# --------------------------------------------------
option(CODE_COVERAGE "Build with --coverage" ON)

# Lists (no quotes → each flag is its own list element)
if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(COVERAGE_COMPILE_FLAGS -O0 -g --coverage)
    set(COVERAGE_LINK_FLAGS    --coverage)
endif()

# --------------------------------------------------
# Header-only libs
# --------------------------------------------------
add_library(buffer_utils INTERFACE)
target_include_directories(buffer_utils INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_library(Catch2 INTERFACE)
target_include_directories(Catch2 INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/test)

# --------------------------------------------------
# Discover & build tests
# --------------------------------------------------
enable_testing()
file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_*.cpp")

foreach(TEST_SRC IN LISTS TEST_SOURCES)
    get_filename_component(TEST_NAME ${TEST_SRC} NAME_WE)
    add_executable(${TEST_NAME} ${TEST_SRC})

    if(CODE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(${TEST_NAME} PRIVATE -O0 -g --coverage)
        target_link_options   (${TEST_NAME} PRIVATE --coverage)
    endif()

    target_link_libraries(${TEST_NAME} PRIVATE buffer_utils Catch2)
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endforeach()


add_custom_target(coverage
    # 1) run all tests – this writes the .gcda files
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure

    # 2-4) run lcov + genhtml **inside MSYS2 Bash**
    COMMAND C:\\msys64\\usr\\bin\\bash.exe -lc
            "lcov  --directory '${CMAKE_BINARY_DIR}' \
                    --capture         --output-file coverage.info  && \
             lcov  --remove  coverage.info '/usr/*' 'C:/msys64/*' \
                    --output-file coverage.cleaned.info            && \
             genhtml coverage.cleaned.info --output-directory coverage_html"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT \"Generating LCOV report in build/coverage_html\"
)