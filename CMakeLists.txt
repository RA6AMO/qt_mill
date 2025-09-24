cmake_minimum_required(VERSION 3.20)

project(cursor_cpp_template LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Qt6: автогенерация MOC/UIC/RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Пусть CMake сможет найти Qt, если путь не передан извне
if(NOT DEFINED CMAKE_PREFIX_PATH)
  set(CMAKE_PREFIX_PATH "C:/Qt/6.9.2/mingw_64")
endif()

## Явная линковка вместо слепого GLOB по всем .a/.dll.a

# Пути к заголовкам сторонних библиотек
set(THIRD_PARTY_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/libs/include")

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_SOURCE_DIR}/src/TreeWidgets/*.cpp"
  "${CMAKE_SOURCE_DIR}/src/DB/*.cpp"
)

# Добавляем заголовочные файлы для правильной работы MOC с Q_OBJECT
file(GLOB_RECURSE PROJECT_HEADERS CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/include/*.h"
  "${CMAKE_SOURCE_DIR}/include/TreeWidgetsHed/*.h"
  "${CMAKE_SOURCE_DIR}/include/DBhead/*.h"
)

add_executable(app ${PROJECT_SOURCES} ${PROJECT_HEADERS})

# Используем найденные папки с заголовками
target_include_directories(app PRIVATE
    "${CMAKE_SOURCE_DIR}/include"
    "${CMAKE_SOURCE_DIR}/include/TreeWidgetsHed"
    "${CMAKE_SOURCE_DIR}/include/DBhead"
    ${THIRD_PARTY_INCLUDE_DIR}
)

# Qt6 модули (Widgets достаточно для классических оконных приложений)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets OpenGLWidgets Sql)

# Линкуем явно нужные библиотеки
# Директории с библиотеками
target_link_directories(app PRIVATE
  "${CMAKE_SOURCE_DIR}/libs/glfw-3.4.bin.WIN64"
  "${CMAKE_SOURCE_DIR}/libs/Glew_migw64"
)

# Если будет нужен GLEW, раскомментируйте glew32 и добавьте define GLEW_STATIC при статической линковке
target_link_libraries(app PRIVATE
  #glew32
  #glfw3        # статическая библиотека libglfw3.a
  #opengl32     # системная OpenGL для Windows
  #gdi32        # требуется GLFW на Windows
  #user32       # системные зависимости GLFW (статическая линковка)
  shell32
  advapi32
  ole32
  uuid
  winmm
  ws2_32
      # если начнете использовать GLEW
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  Qt6::OpenGLWidgets
  Qt6::Sql
)

if(MSVC)
  target_compile_options(app PRIVATE /W4 /permissive-)
else()
  target_compile_options(app PRIVATE -Wall -Wextra -Wpedantic)
endif()
