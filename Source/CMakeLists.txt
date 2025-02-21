#
# Copyright 2025 Andrew Rogers
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
include(FetchContent)

FetchContent_Declare(
  elfio
  GIT_REPOSITORY    https://github.com/serge1/ELFIO.git
  GIT_TAG           Release_3.12
  FIND_PACKAGE_ARGS CONFIG
)
FetchContent_MakeAvailable(elfio)

block(SCOPE_FOR VARIABLES)
  set(CAPSTONE_BUILD_TESTS OFF CACHE INTERNAL "")
  set(CAPSTONE_BUILD_SHARED OFF CACHE INTERNAL "")
  set(CAPSTONE_ARCHITECTURE_DEFAULT OFF CACHE INTERNAL "")
  if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64")
    set(CAPSTONE_X86_SUPPORT ON CACHE INTERNAL "")
  elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64")
    set(CAPSTONE_ARM64_SUPPORT ON CACHE INTERNAL "")
  endif()
  FetchContent_Declare(
    capstone
    GIT_REPOSITORY    https://github.com/capstone-engine/capstone
    GIT_TAG           5.0.5
    FIND_PACKAGE_ARGS CONFIG
  )
  FetchContent_MakeAvailable(capstone)
endblock()

add_library(libInterject STATIC
  disassembler.cxx
  event.cxx
  memory_map.cxx
  modules.cxx
  symbols.cxx
  threads.cxx
  trampoline.cxx
  transaction.cxx
  unwind.cxx
)
target_include_directories(libInterject PUBLIC include)
target_include_directories(libInterject PRIVATE ${elfio_SOURCE_DIR})
target_link_libraries(libInterject PRIVATE capstone)
