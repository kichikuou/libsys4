cmake_minimum_required(VERSION 3.10)
project(libsys4 LANGUAGES C)
set(CMAKE_C_STANDARD 11)

# Why NO_CMAKE_FIND_ROOT_PATH is needed?
# https://gitlab.kitware.com/cmake/cmake/-/issues/16870
find_package(libjpeg-turbo REQUIRED NO_CMAKE_FIND_ROOT_PATH)
find_package(WebP REQUIRED NO_CMAKE_FIND_ROOT_PATH)
include(${CMAKE_STAGING_PREFIX}/lib/libpng/libpng16.cmake)

find_package(FLEX)
find_package(BISON)

add_library(sys4 STATIC)

target_compile_definitions(sys4 PRIVATE _DEFAULT_SOURCE)
target_include_directories(sys4 PUBLIC include PRIVATE src)

target_sources(sys4 PRIVATE
  src/acx.c
  src/afa.c
  src/afa3.c
  src/ain.c
  src/ajp.c
  src/ald.c
  src/alk.c
  src/archive.c
  src/buffer.c
  src/cg.c
  src/dasm.c
  src/dcf.c
  src/dlf.c
  src/ex.c
  src/file.c
  src/flat.c
  src/fnl.c
  src/hashtable.c
  src/ini.c
  src/instructions.c
  src/jpeg.c
  src/mt19937int.c
  src/pms.c
  src/png.c
  src/qnt.c
  src/string.c
  src/system.c
  src/utfsjis.c
  src/webp.c
  )

FLEX_TARGET(ini_lexer src/ini_lexer.l  ${CMAKE_CURRENT_BINARY_DIR}/ini_lexer.yy.c)
BISON_TARGET(ini_parser src/ini_parser.y ${CMAKE_CURRENT_BINARY_DIR}/ini_parser.tab.c
  DEFINES_FILE ${CMAKE_CURRENT_BINARY_DIR}/ini_parser.tab.h)
ADD_FLEX_BISON_DEPENDENCY(ini_lexer ini_parser)

target_include_directories(sys4 PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_sources(sys4 PRIVATE
  ${FLEX_ini_lexer_OUTPUTS}
  ${BISON_ini_parser_OUTPUTS}
  )

target_link_libraries(sys4 PRIVATE
  m z log libjpeg-turbo::turbojpeg-static WebP::webp png_static)
