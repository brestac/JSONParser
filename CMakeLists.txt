cmake_minimum_required(VERSION 3.15)
project(JSONParser CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Ressources JSON à copier dans le répertoire de build ──────────────────────
file(GLOB JSON_RESOURCES "${CMAKE_SOURCE_DIR}/examples/JSONParserTest/*.json")

# ── desktop-test (release, clang, O2) ────────────────────────────────────────
add_executable(desktop-test
    tests/desktop.cpp
)
target_include_directories(desktop-test PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_options(desktop-test PRIVATE
    -O2 -Wall -Werror -Werror=unused-variable -Wno-format-security
    -DJSON_DEBUG_LEVEL_NONE=0 -DJSON_DEBUG_LEVEL_INFO=1
    -DJSON_DEBUG_LEVEL_WARNING=2 -DJSON_DEBUG_LEVEL_ERROR=3
)
foreach(res ${JSON_RESOURCES})
    get_filename_component(res_name "${res}" NAME)
    add_custom_command(TARGET desktop-test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${res}" "$<TARGET_FILE_DIR:desktop-test>/${res_name}"
    )
endforeach()

# ── desktop-test-arduino (mock ESP8266, g++, Os, no-rtti, no-exceptions) ─────
add_executable(desktop-test-arduino
    tests/desktop.cpp
)
target_include_directories(desktop-test-arduino PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_options(desktop-test-arduino PRIVATE
    -Os -Wall -Werror -Werror=unused-variable -Wno-format-security
    -Wno-unknown-pragmas
    -fno-rtti -falign-functions=4 -ffunction-sections -fdata-sections -fno-exceptions
    -DJSON_DEBUG_LEVEL_NONE=0 -DJSON_DEBUG_LEVEL_INFO=1
    -DJSON_DEBUG_LEVEL_WARNING=2 -DJSON_DEBUG_LEVEL_ERROR=3
)
foreach(res ${JSON_RESOURCES})
    get_filename_component(res_name "${res}" NAME)
    add_custom_command(TARGET desktop-test-arduino POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${res}" "$<TARGET_FILE_DIR:desktop-test-arduino>/${res_name}"
    )
endforeach()

# ── desktop-test-debug (debug, clang, O0, sanitizers) ────────────────────────
add_executable(desktop-test-debug
    tests/desktop.cpp
)
target_include_directories(desktop-test-debug PRIVATE ${CMAKE_SOURCE_DIR})
target_compile_options(desktop-test-debug PRIVATE
    -O0 -Wall -Werror -Werror=unused-variable -Wno-format-security
    -Wextra -Wpedantic
    -Wno-gnu-zero-variadic-macro-arguments -Wno-variadic-macros -Wno-vla-extension
    -ferror-limit=50
    -DJSON_DEBUG_LEVEL_NONE=0 -DJSON_DEBUG_LEVEL_INFO=1
    -DJSON_DEBUG_LEVEL_WARNING=2 -DJSON_DEBUG_LEVEL_ERROR=3
)
foreach(res ${JSON_RESOURCES})
    get_filename_component(res_name "${res}" NAME)
    add_custom_command(TARGET desktop-test-debug POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${res}" "$<TARGET_FILE_DIR:desktop-test-debug>/${res_name}"
    )
endforeach()
