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

#include <iostream>

#include <pthread.h>
#include <unistd.h>

#include <transaction.hxx>

using namespace Interject;

extern "C" __attribute__((noinline)) ssize_t test_fn_add(ssize_t arg1,
                                                         ssize_t arg2) {
  return arg1 + arg2;
}

extern "C" __attribute__((noinline)) ssize_t test_fn_sub(size_t arg1,
                                                         ssize_t arg2) {
  return arg1 - arg2;
}

ssize_t (*test_fn_add_trampoline)(ssize_t arg1, ssize_t arg2);
ssize_t (*test_fn_sub_trampoline)(ssize_t arg1, ssize_t arg2);

ssize_t hook_fn_add(ssize_t arg1, ssize_t arg2) { return arg1 + arg2; }

ssize_t hook_fn_sub(ssize_t arg1, ssize_t arg2) { return arg1 - arg2; }

TEST_CASE("Create and abort transaction", "[transaction]") {
  Interject::Transaction txn =
      Transaction::Builder()
          .add("test_fn_add", hook_fn_sub, &test_fn_add_trampoline)
          .add("test_fn_sub", hook_fn_add, &test_fn_sub_trampoline)
          .build();
  CHECK(txn.prepare() == Transaction::ResultCode::Success);
  CHECK(txn.commit() == Transaction::ResultCode::Success);

  CHECK(test_fn_add(1, 1) == 0);
  CHECK(test_fn_sub(1, 1) == 2);
}

extern "C" __attribute__((noinline)) bool test_fn_return_bool(bool value) {
  return value;
}

extern "C" __attribute__((noinline)) bool test_fn_return_not_bool(bool value) {
  return !value;
}

bool (*test_fn_return_true_trampoline)(bool);

static void *testThread(void *arg) {
  size_t value = 0;
  // This thread will run until test_fn_return_bool is patched to call
  // test_fn_return_not_bool instead.
  while (test_fn_return_bool(true)) {
    value += 1;
  }
  return nullptr;
}

TEST_CASE("Multiple threads during transaction", "[thread, transaction]") {
  Interject::Transaction txn =
      Transaction::Builder()
          .add("test_fn_return_bool", test_fn_return_not_bool,
               &test_fn_return_true_trampoline)
          .build();
  CHECK(txn.prepare() == Transaction::ResultCode::Success);

  pthread_attr_t attr;
  CHECK_FALSE(pthread_attr_init(&attr));

  // Create many threads that are continually calling the target functions
  // in a tight loop in an attempt to race with the commit call patching the
  // code. If patching occurs concurrently with execution, it will lead to
  // undefined behavior and hopefully crash the application with SIGILL or
  // SIGSEGV. Since Transaction::commit intends to avoid this situation, it
  // should proceed without issue.
  constexpr size_t threadCount = 50;
  pthread_t threadIds[threadCount];
  for (size_t i = 0; i < threadCount; i++) {
    CHECK_FALSE(pthread_create(&threadIds[i], &attr, testThread, nullptr));
  }

  // Give the threads a chance to start running before committing the patches.
  ::usleep(1000);
  CHECK(txn.commit() == Transaction::ResultCode::Success);

  for (size_t i = 0; i < threadCount; i++) {
    // Threads will exit after test_fn_return_bool has been patched to call
    // test_fn_return_not_bool.
    CHECK_FALSE(pthread_join(threadIds[i], nullptr));
  }
}
