cmake_minimum_required(VERSION 3.15)
project(JSONParserProject CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# -- set(CMAKE_CXX_COMPILER "clang++")

# Créer une bibliothèque d'en-têtes (interface)
add_library(jsonparser INTERFACE)

# Indiquer le dossier contenant les fichiers .h pour la compilation
target_include_directories(jsonparser INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

set(jsonparser INTERFACE JSON_DEBUG_LEVEL_NONE 0)
set(jsonparser INTERFACE JSON_DEBUG_LEVEL_INFO 1)
set(jsonparser INTERFACE JSON_DEBUG_LEVEL_WARNING 2)
set(jsonparser INTERFACE JSON_DEBUG_LEVEL_ERROR 3)

# Créer une bibliothèque d'en-têtes (interface)
add_library(jsonparser-arduino INTERFACE)

# Indiquer le dossier contenant les fichiers .h pour la compilation
target_include_directories(jsonparser-arduino INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
)

set(jsonparser-arduino INTERFACE JSON_DEBUG_LEVEL_NONE 0)
set(jsonparser-arduino INTERFACE JSON_DEBUG_LEVEL_INFO 1)
set(jsonparser-arduino INTERFACE JSON_DEBUG_LEVEL_WARNING 2)
set(jsonparser-arduino INTERFACE JSON_DEBUG_LEVEL_ERROR 3)
target_compile_options(jsonparser-arduino INTERFACE
    -DCMAKE_CXX_COMPILER=g++
    -Os
    -std=gnu++17
    -fno-rtti
    -falign-functions=4
    -ffunction-sections
    -fdata-sections
    -fno-exceptions
)

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.15.3
)
FetchContent_MakeAvailable(Catch2)

FetchContent_Declare(
  simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG  tags/v3.6.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(simdjson)

FetchContent_Declare(
  json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG v3.12.0
)
FetchContent_MakeAvailable(json)

FetchContent_Declare(
  ArduinoJson
  GIT_REPOSITORY https://github.com/bblanchon/ArduinoJson.git
  GIT_TAG v7.4.3
)
FetchContent_MakeAvailable(ArduinoJson)

# Déclarer et récupérer RapidJSON depuis GitHub
FetchContent_Declare(
    rapidjson_src
    GIT_REPOSITORY https://github.com/Tencent/rapidjson.git
    GIT_TAG        v1.1.0 # ou un commit SHA récent
)
FetchContent_MakeAvailable(rapidjson_src)

add_library(rapidjson INTERFACE)
target_include_directories(rapidjson INTERFACE ${rapidjson_src_SOURCE_DIR}/include)

set(rapidjson INTERFACE RAPIDJSON_BUILD_DOC OFF)
set(rapidjson INTERFACE RAPIDJSON_BUILD_TERS OFF)
set(rapidjson INTERFACE RAPIDJSON_BUILD_TESTS OFF)
set(rapidjson INTERFACE RAPIDJSON_BUILD_EXAMPLES OFF)
target_compile_options(rapidjson INTERFACE
    -Wno-deprecated-declarations
    -Wno-class-memaccess
)

# ── Ressources JSON à copier dans le répertoire de build ──────────────────────
file(GLOB JSON_RESOURCES "${CMAKE_SOURCE_DIR}/examples/JSONParserTest/*.*json")

add_compile_options(
    -Wall
    -Werror
    -Werror=unused-variable
)

function(copy_resources CIBLE)
    foreach(res ${JSON_RESOURCES})
        get_filename_component(res_name "${res}" NAME)
        add_custom_command(TARGET ${CIBLE} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${res}" "$<TARGET_FILE_DIR:${CIBLE}>/${res_name}"
        )
    endforeach()
endfunction()

add_executable(main
    tests/main.cpp
)
target_include_directories(main PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(main PRIVATE jsonparser)
target_compile_options(main PRIVATE
    -O2
    -std=gnu++17
)
copy_resources(main)

# ── release (release, clang, O2) ────────────────────────────────────────
add_executable(release
    tests/test.cpp
)
target_include_directories(release PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(release PRIVATE Catch2::Catch2WithMain jsonparser)
target_compile_options(release PRIVATE
    -O2
    -std=gnu++17
    -ftemplate-backtrace-limit=0
)
copy_resources(release)

# ── release (release benchmarks, clang, O2) ────────────────────────────────────────
add_executable(release-perf
    tests/reporter.cpp
    tests/perf.cpp
)

target_include_directories(release-perf PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(release-perf PRIVATE Catch2::Catch2WithMain jsonparser simdjson rapidjson nlohmann_json::nlohmann_json ArduinoJson)

target_compile_options(release-perf PRIVATE
    -O2
    -std=gnu++17
)
copy_resources(release-perf)

# ── debug (debug, clang, O0, sanitizers) ────────────────────────
add_executable(debug
    tests/test.cpp
)

target_include_directories(debug PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(debug PRIVATE Catch2::Catch2WithMain jsonparser)

target_compile_options(debug PRIVATE
    -O0
    -std=gnu++17
    -Wno-cpp
    -Wno-variadic-macros
    -Wextra
    -Wpedantic
    -Wdangling-pointer
     -error-limit=50
)
foreach(res ${JSON_RESOURCES})
    get_filename_component(res_name "${res}" NAME)
    add_custom_command(TARGET debug POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${res}" "$<TARGET_FILE_DIR:debug>/${res_name}"
    )
endforeach()

# ── release-arduino (mock ESP8266, g++, Os, no-rtti, no-exceptions) ─────
add_executable(release-arduino
    tests/test.cpp
)
target_compile_options(release-arduino PRIVATE
    -O2
    -std=gnu++17
)
target_include_directories(release-arduino PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(release-arduino PRIVATE Catch2::Catch2WithMain jsonparser-arduino)

copy_resources(release-arduino)

# -- faster -o3 c++23
add_executable(faster
    tests/test.cpp
)

target_include_directories(faster PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(faster PRIVATE Catch2::Catch2WithMain jsonparser)

target_compile_options(faster PRIVATE
    -O3
    -std=gnu++23
)
copy_resources(faster)
