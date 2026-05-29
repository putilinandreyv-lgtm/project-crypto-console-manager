cmake_minimum_required(VERSION 3.15)

# 1. Имя проекта и язык
project("project-crypto-console-manager" LANGUAGES CXX)

# 2. Настройка стандарта C++ (C++17 требуется для работы json и std::any)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Направляем все исполняемые файлы и DLL в одну общую папку внутри build/bin,
# чтобы Windows не теряла .dll файлы при запуске .exe
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 3. Подключение модуля FetchContent для управления зависимостями
include(FetchContent)

# -----------------------------------------------------------------------------
# Зависимость 1: nlohmann_json (Умный поиск / скачивание)
# -----------------------------------------------------------------------------
message(STATUS "Проверка наличия nlohmann_json на компьютере...")
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    FIND_PACKAGE_ARGS NAMES nlohmann_json
)
FetchContent_MakeAvailable(json)

# -----------------------------------------------------------------------------
# Зависимость 2: libcurl (Умный поиск / скачивание)
# -----------------------------------------------------------------------------
message(STATUS "Проверка наличия libcurl на компьютере...")

# Оптимизируем опции сборки curl на случай, если придется скачивать исходники
set(BUILD_CURL_EXE OFF CACHE INTERNAL "Disable curl executable")
set(BUILD_TESTING OFF CACHE INTERNAL "Disable curl tests")
set(CURL_ENABLE_SSL ON CACHE INTERNAL "Enable SSL support for HTTPS")
set(HTTP_ONLY ON CACHE INTERNAL "Build only HTTP/HTTPS protocols")

FetchContent_Declare(
    curl
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG        curl-8_7_1
    FIND_PACKAGE_ARGS NAMES CURL
)
FetchContent_MakeAvailable(curl)

# -----------------------------------------------------------------------------
# 4. Создание основного исполняемого файла проекта
# -----------------------------------------------------------------------------
add_executable(${PROJECT_NAME} ConsoleApplication1.cpp)

# Линковка библиотек к основному проекту
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        nlohmann_json::nlohmann_json
        libcurl
)

# -----------------------------------------------------------------------------
# Блок тестирования проекта (Запуск через ctest)
# -----------------------------------------------------------------------------
enable_testing()

# Фикс политики версий для обратной совместимости со старыми CMake-скриптами библиотек
set(CMAKE_POLICY_VERSION_MINIMUM "3.5")

# Зависимость 3: doctest (Умный поиск / скачивание)
message(STATUS "Проверка наличия Doctest на компьютере...")
FetchContent_Declare(
    doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
    FIND_PACKAGE_ARGS NAMES doctest
)
FetchContent_MakeAvailable(doctest)

# Создаем ОТДЕЛЬНЫЙ исполняемый файл для тестов
add_executable(run_tests tests.cpp)

target_link_libraries(run_tests
    PRIVATE
        doctest::doctest
        nlohmann_json::nlohmann_json
        libcurl
)

# Регистрация тестового файла в движке ctest
add_test(NAME CryptoManagerUnitTests COMMAND run_tests)