cmake_minimum_required(VERSION 3.15)

project("project-crypto-console-manager" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    FIND_PACKAGE_ARGS NAMES nlohmann_json
)
FetchContent_MakeAvailable(json)

set(BUILD_CURL_EXE OFF CACHE INTERNAL "")
set(BUILD_TESTING OFF CACHE INTERNAL "")
set(CURL_ENABLE_SSL ON CACHE INTERNAL "")
set(HTTP_ONLY ON CACHE INTERNAL "")

FetchContent_Declare(
    curl
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG        curl-8_7_1
    FIND_PACKAGE_ARGS NAMES CURL
)
FetchContent_MakeAvailable(curl)

if(NOT TARGET CURL::libcurl)
    if(TARGET libcurl_shared)
        add_library(CURL::libcurl ALIAS libcurl_shared)
    elseif(TARGET libcurl_static)
        add_library(CURL::libcurl ALIAS libcurl_static)
    endif()
endif()

add_executable(${PROJECT_NAME} ConsoleApplication1.cpp)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        nlohmann_json::nlohmann_json
        CURL::libcurl
)

enable_testing()
set(CMAKE_POLICY_VERSION_MINIMUM "3.5")

FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
    FIND_PACKAGE_ARGS NAMES doctest
)
FetchContent_MakeAvailable(doctest)

add_executable(run_tests tests.cpp ConsoleApplication1.cpp)
target_compile_definitions(run_tests PRIVATE TEST_BUILD)
target_link_libraries(run_tests
    PRIVATE
        doctest::doctest
        nlohmann_json::nlohmann_json
        CURL::libcurl
)

add_test(NAME CryptoManagerUnitTests COMMAND run_tests)