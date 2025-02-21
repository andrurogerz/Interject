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
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.8.0
)
FetchContent_MakeAvailable(Catch2)

add_executable(InterjectTests
  functions.c
  memory_map_tests.cxx
  symbol_tests.cxx
  transaction_tests.cxx
)
target_include_directories(InterjectTests PRIVATE ${CMAKE_SOURCE_DIR}/Source)
target_link_libraries(InterjectTests PRIVATE libInterject)
target_link_libraries(InterjectTests PRIVATE Catch2::Catch2WithMain)

enable_testing()
add_test(NAME InterjectTests COMMAND InterjectTests)
