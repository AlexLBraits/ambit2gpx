cmake_minimum_required (VERSION 3.12)
project (ambit)

include_directories(hidapi)

set(
  ALL_SOURCE

  crc16.c
  debug.c
  libambit.c
  personal.c
  pmem20.c
  protocol.c
)

if (APPLE)
  set(
    ALL_SOURCE
    ${ALL_SOURCE}
    hid_mac.c
  )

  set(
    USED_FRAMEWORKS
    "-framework IOKit"
    "-framework CoreFoundation"
    )
elseif (WIN32)
  set(
    ALL_SOURCE
    ${ALL_SOURCE}
    hid_win.c
  )

  set(
      USED_FRAMEWORKS
      ws2_32.lib
      setupapi.lib
      )
endif ()

add_library (${PROJECT_NAME} STATIC ${ALL_SOURCE})
target_link_libraries(${PROJECT_NAME} ${USED_FRAMEWORKS})
