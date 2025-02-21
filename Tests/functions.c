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

#include <stdbool.h>
#include <stdlib.h>

__attribute__((noinline, used))
size_t count_set_bits(size_t n) {
  size_t count = 0;
  while (n) {
    n &= (n - 1);
    count++;
  }
  return count;
}

__attribute__((noinline, used))
size_t fibonacci(size_t n) {
  if (n <= 1) {
    return n;
  }
  size_t a = 0;
  size_t b = 1;
  for (size_t i = 2; i <= n; ++i) {
    size_t temp = b;
    b += a;
    a = temp;
  }
  return b;
}

__attribute__((noinline, used))
size_t isqrt(size_t n) {
  if (n == 0)  {
    return 0;
  }
  size_t x = n;
  size_t y = (x + 1) / 2;
  while (y < x) {
    x = y;
    y = (x + n / x) / 2;
  }
  return x;
}

__attribute__((noinline, used))
size_t sum_of_digits(size_t n) {
  size_t sum = 0;
  while (n > 0) {
    sum += n % 10;
    n /= 10;
  }
  return sum;
}

__attribute__((noinline, used))
size_t reverse_digits(size_t n) {
  size_t reversed = 0;
  while (n > 0) {
    reversed = reversed * 10 + n % 10;
    n /= 10;
  }
  return reversed;
}

__attribute__((noinline, used))
size_t factorial(size_t n) {
  size_t result = 1;
  for (size_t i = 1; i <= n; ++i) {
    result *= i;
  }
  return result;
}

// disable optimizations so that these functions are large enough to be patched
#pragma clang optimize off
__attribute__((noinline, used))
bool test_fn_return_bool(bool value) {
  return value;
}

__attribute__((noinline, used))
bool test_fn_return_not_bool(bool value) {
  return !value;
}
#pragma clang optimize on
