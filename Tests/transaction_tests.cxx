/*
 * Copyright 2025 Andrew Rogers
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define CATCH_CONFIG_MAIN
#include <catch2/catch_test_macros.hpp>

#include <transaction.hxx>

using namespace Interject;

extern "C" ssize_t test_fn_add(ssize_t arg1, ssize_t arg2) {
  return arg1 + arg2;
}

extern "C" ssize_t test_fn_sub(size_t arg1, ssize_t arg2) {
  return arg1 - arg2;
}

ssize_t (*test_fn_add_trampoline)(ssize_t arg1, ssize_t arg2);
ssize_t (*test_fn_sub_trampoline)(ssize_t arg1, ssize_t arg2);

ssize_t hook_fn_add(ssize_t arg1, ssize_t arg2) {
  // TODO call test_fn_add_trampoline
  return 7;
}

ssize_t hook_fn_sub(ssize_t arg1, ssize_t arg2) {
  // TODO call test_fn_sub_trampoline
  return 12;
}

TEST_CASE("Create and abort transaction", "[transaction]") {
  //auto hook = hook_fn_add;
  Interject::Transaction txn = Transaction::Builder()
    .add("test_fn_add", hook_fn_add, &test_fn_add_trampoline)
    .add("test_fn_sub", hook_fn_sub, &test_fn_sub_trampoline)
    .build();
  CHECK(txn.prepare() == Transaction::ResultCode::Success);
  CHECK(txn.commit() == Transaction::ResultCode::Success);
}
