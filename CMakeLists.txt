cmake_minimum_required(VERSION 3.16)

project(2dx-gsm VERSION 1.3.0.0 LANGUAGES CXX)

add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES})

target_sources(${PROJECT_NAME} PRIVATE
    src/api.cc
    src/bm2dx.cc
    src/bm2dx_offsets.cc
    src/hooks.cc
    src/config.cc
    src/dllmain.cc
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_23)
target_compile_definitions(${PROJECT_NAME} PRIVATE -DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN)

find_package(safetyhook CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}
    spdlog::spdlog
    safetyhook::safetyhook
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_BINARY_DIR}/macros
)

target_precompile_headers(${PROJECT_NAME} PRIVATE
    <spdlog/spdlog.h>
)

if (MINGW)
    set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
    target_compile_options(${PROJECT_NAME} PRIVATE -fvisibility=hidden)
endif()

file(GLOB VERSIONS cmake/2dx-gsm.*.cmake)

foreach(VERSION_PATH ${VERSIONS})
    get_filename_component(VERSION_FILE ${VERSION_PATH} NAME)
    message(STATUS "Adding game support from ${VERSION_FILE}...")

    include(${VERSION_PATH})

    set(VERSION_FILENAME "macros/version.${EXPECTED_BM2DX_PRODUCT_VERSION}.cc")
    string(REPLACE "-" "_" EXPECTED_BM2DX_PRODUCT_VERSION ${EXPECTED_BM2DX_PRODUCT_VERSION})
    configure_file(src/version.cc.in ${VERSION_FILENAME} @ONLY)
    target_sources(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${VERSION_FILENAME})
endforeach()

configure_file(src/version.h.in macros/version.h @ONLY)