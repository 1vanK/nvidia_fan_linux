cmake_minimum_required(VERSION 3.16)

project(nvidia_fan)

# Если используется одноконфигурационный генератор
# и конфигурация не указана
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
    # то конфигурацией по умолчанию будет Release
    set(CMAKE_BUILD_TYPE Release)

    # Нельзя оставлять переменную CMAKE_BUILD_TYPE пустой,
    # так как при этом не будут заданы флаги GCC:
    # * Пустая строка: CXX_FLAGS = -std=c++23
    # * Release: CXX_FLAGS = -O3 -DNDEBUG -std=c++23
    # * Debug: CXX_FLAGS = -g -std=c++23
    # * RelWithDebInfo: CXX_FLAGS = -O2 -g -DNDEBUG -std=c++23
    # * MinSizeRel: CXX_FLAGS = -Os -DNDEBUG -std=c++23
    # Флаги можно посмотреть в файле build/CMakeFiles/nvidia_fan_linux.dir/flags.make
endif()

# Версия стандарта C++
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Название таргета
set(target_name nvidia_fan)

# Создаём список файлов
file(GLOB_RECURSE source_files src/*.cpp src/*.hpp)

# Создаём приложение
add_executable(${target_name} ${source_files})

# Выводим больше предупреждений
target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)

# Подключаем библиотеки
target_link_libraries(${target_name} PRIVATE -lnvidia-ml)
