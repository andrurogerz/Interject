cmake_minimum_required(VERSION 3.30)
project(AltDroid LANGUAGES C CXX)

include(FetchContent)

FetchContent_Declare(
  elfio
  GIT_REPOSITORY    https://github.com/serge1/ELFIO.git
  GIT_TAG           Release_3.12
  FIND_PACKAGE_ARGS CONFIG
)
FetchContent_MakeAvailable(elfio)

add_library(libAltDroid STATIC
  src/lib/link_map.cxx
)
target_include_directories(libAltDroid PUBLIC src/include)
target_include_directories(libAltDroid PUBLIC ${elfio_SOURCE_DIR})

set_target_properties(libAltDroid PROPERTIES CXX_STANDARD 17)
set_target_properties(libAltDroid PROPERTIES CXX_STANDARD_REQUIRED YES)
set_target_properties(libAltDroid PROPERTIES CXX_EXTENSIONS NO)

add_executable(test
  src/test/main.cxx
)
target_link_libraries(test PRIVATE libAltDroid)

set_target_properties(test PROPERTIES CXX_STANDARD 17)
set_target_properties(test PROPERTIES CXX_STANDARD_REQUIRED YES)
set_target_properties(test PROPERTIES CXX_EXTENSIONS NO)
