cmake_minimum_required(VERSION 3.15)

# 1. Имя проекта и язык
project("project-crypto-console-manager" LANGUAGES CXX)

# 2. Настройка стандарта C++ (требуется C++17 для std::any, json и других фич)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 3. Подключение модуля FetchContent для управления зависимостями
include(FetchContent)

# -----------------------------------------------------------------------------
# Зависимость 1: nlohmann_json
# -----------------------------------------------------------------------------
message(STATUS "FetchContent: Настройка nlohmann_json...")
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3 # Стабильная версия библиотеки
)
FetchContent_MakeAvailable(json)

# -----------------------------------------------------------------------------
# Зависимость 2: libcurl (для сетевых запросов к крипто-API)
# -----------------------------------------------------------------------------
message(STATUS "FetchContent: Настройка libcurl...")

# Оптимизируем сборку curl, чтобы не тратить время на лишнее
set(BUILD_CURL_EXE OFF CACHE INTERNAL "Disable curl executable")
set(BUILD_TESTING OFF CACHE INTERNAL "Disable curl tests")
set(CURL_ENABLE_SSL ON CACHE INTERNAL "Enable SSL support for HTTPS")
set(HTTP_ONLY ON CACHE INTERNAL "Build only HTTP/HTTPS protocols")

FetchContent_Declare(
    curl
    GIT_REPOSITORY https://github.com/curl/curl.git
    GIT_TAG        curl-8_7_1 # Надежный релиз curl
)
FetchContent_MakeAvailable(curl)

# -----------------------------------------------------------------------------
# 4. Создание исполняемого файла проекта
# -----------------------------------------------------------------------------
# Сюда при необходимости можно через пробел добавлять другие ваши .cpp файлы
add_executable(${PROJECT_NAME} ConsoleApplication1.cpp)

# -----------------------------------------------------------------------------
# 5. Связывание (линковка) библиотек с вашим проектом
# -----------------------------------------------------------------------------
# Так как библиотеки собираются из исходников внутри проекта, 
# их таргеты называются именно так:
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        nlohmann_json::nlohmann_json
        libcurl
)