cmake_minimum_required (VERSION 3.12)
project (ambit2gpx)

add_subdirectory(3rdparty/libambit ${CMAKE_BINARY_DIR}/3rdparty/libambit)

include_directories(3rdparty/libambit)

add_executable(
  ${PROJECT_NAME} src/main.c
)

target_link_libraries(
  ${PROJECT_NAME} ambit
)
