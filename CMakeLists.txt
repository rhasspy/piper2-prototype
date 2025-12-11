cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(piper2)

find_package(ICU COMPONENTS uc i18n REQUIRED)

set(THIRD_PARTY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# ---- onnxruntime ----

# Look for onnxruntime files in <root>/lib
if(NOT DEFINED ONNXRUNTIME_DIR)
    if(NOT DEFINED ONNXRUNTIME_VERSION)
        set(ONNXRUNTIME_VERSION "1.23.2")
    endif()

    if(WIN32)
        # Windows x86-64
        set(ONNXRUNTIME_PREFIX "onnxruntime-win-x64-${ONNXRUNTIME_VERSION}")
        set(ONNXRUNTIME_EXT "zip")
    elseif (APPLE)
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64)
            # MacOS x86-64
            set(ONNXRUNTIME_PREFIX "onnxruntime-osx-x86_64-${ONNXRUNTIME_VERSION}")
        elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL arm64)
            # MacOS Apple Silicon
            set(ONNXRUNTIME_PREFIX "onnxruntime-osx-arm64-${ONNXRUNTIME_VERSION}")
        else()
            message(FATAL_ERROR "Unsupported architecture for onnxruntime")
        endif()

        set(ONNXRUNTIME_EXT "tgz")
    else()
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL x86_64)
            # Linux x86-64
            set(ONNXRUNTIME_PREFIX "onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}")
        elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL aarch64)
            # Linux ARM 64-bit
            set(ONNXRUNTIME_PREFIX "onnxruntime-linux-aarch64-${ONNXRUNTIME_VERSION}")
        elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL armv7l)
            # Linux ARM 32-bit
            set(ONNXRUNTIME_PREFIX "onnxruntime-linux-arm32-${ONNXRUNTIME_VERSION}")
            set(ONNXRUNTIME_URL "https://github.com/synesthesiam/prebuilt-apps/releases/download/v1.0/onnxruntime-linux-arm32-${ONNXRUNTIME_VERSION}.tgz")
        else()
            message(FATAL_ERROR "Unsupported architecture for onnxruntime")
        endif()

        set(ONNXRUNTIME_EXT "tgz")
    endif()

    if(NOT DEFINED ONNXRUNTIME_URL)
        set(ONNXRUNTIME_URL "https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/${ONNXRUNTIME_PREFIX}.${ONNXRUNTIME_EXT}")
    endif()

    set(ONNXRUNTIME_FILENAME "${ONNXRUNTIME_PREFIX}.${ONNXRUNTIME_EXT}")
    set(ONNXRUNTIME_DIR "${THIRD_PARTY_DIR}/${ONNXRUNTIME_PREFIX}")

    if(NOT EXISTS "${ONNXRUNTIME_DIR}")
        if(NOT EXISTS "download/${ONNXRUNTIME_FILENAME}")
            # Download onnxruntime release
            message("Downloading ${ONNXRUNTIME_URL}")
            file(DOWNLOAD "${ONNXRUNTIME_URL}" "download/${ONNXRUNTIME_FILENAME}")
        endif()

        # Extract .zip or .tgz to a directory like lib/onnxruntime-linux-x64-1.23.2/
        file(ARCHIVE_EXTRACT INPUT "download/${ONNXRUNTIME_FILENAME}" DESTINATION "${THIRD_PARTY_DIR}")
    endif()
endif()

# add_library(onnxruntime STATIC IMPORTED)
# set_target_properties(onnxruntime PROPERTIES
#     IMPORTED_LOCATION "${CMAKE_SOURCE_DIR}/third_party/onnxruntime/lib/libonnxruntime.a"
#     INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_SOURCE_DIR}/third_party/onnxruntime/include"
# )

# ---- libpiper2 ---

set(LIBPIPER2_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libpiper2")
add_library(piper2 SHARED
    "${LIBPIPER2_SOURCE_DIR}/src/piper2.cpp"
)

target_include_directories(piper2 PUBLIC
    "${ONNXRUNTIME_DIR}/include"
    "${THIRD_PARTY_DIR}/json/include"
    "${LIBPIPER2_SOURCE_DIR}/include"
)
target_link_directories(piper2 PUBLIC
    "${ONNXRUNTIME_DIR}/lib"
)
target_link_libraries(piper2
    onnxruntime
)

# add_executable(piper2 main.cpp)
# target_link_libraries(piper2 PRIVATE onnxruntime)

# include_directories(
#     "${CMAKE_SOURCE_DIR}/include"
#     "${CMAKE_SOURCE_DIR}/third_party/dr_wav/include"
#     "${CMAKE_SOURCE_DIR}/third_party/json/include"
#     "${CMAKE_SOURCE_DIR}/third_party/uni_algo/include"
# )
