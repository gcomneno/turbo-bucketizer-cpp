cmake_minimum_required(VERSION 3.14)

project(turbo-bucketizer-cpp
    VERSION 0.1.0
    DESCRIPTION "Deterministic IPv4 bucketizer in modern C++17"
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTING "Build tests" ON)

# ---- Core library ----
add_library(tb_core
    src/bucket_engine.cpp
    src/stats.cpp
    src/utils.cpp
)

target_include_directories(tb_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(tb_core PUBLIC cxx_std_17)

# ---- CLI application ----
add_executable(tb_cli
    apps/tb_cli.cpp
)

target_link_libraries(tb_cli
    PRIVATE
        tb_core
)

# ---- Tests ----
if(BUILD_TESTING)
    include(CTest)

    add_executable(tb_tests
        tests/test_bucketizer.cpp
        tests/catch_amalgamated.cpp
    )

    target_include_directories(tb_tests
        PRIVATE
            ${CMAKE_CURRENT_SOURCE_DIR}/include
            ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    target_link_libraries(tb_tests
        PRIVATE
            tb_core
    )

    add_test(NAME tb_tests COMMAND tb_tests)
endif()

include(GNUInstallDirs)

install(TARGETS tb_core tb_cli
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
