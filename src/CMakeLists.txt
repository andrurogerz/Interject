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

add_library(libRevise STATIC
  lib/modules.cxx
  lib/symbols.cxx
)
target_include_directories(libRevise PUBLIC include)
target_include_directories(libRevise PRIVATE ${elfio_SOURCE_DIR})
