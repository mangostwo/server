file(GLOB sources *.c)
list(REMOVE_ITEM sources ${CMAKE_CURRENT_SOURCE_DIR}/lua.c)
list(REMOVE_ITEM sources ${CMAKE_CURRENT_SOURCE_DIR}/luac.c)

set(lua_STAT_SRCS
  ${sources}
)

include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
)

add_library(lualib STATIC ${lua_STAT_SRCS})