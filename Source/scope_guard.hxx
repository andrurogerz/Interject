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

#pragma once

#include <type_traits>
#include <utility>

namespace Interject::ScopeGuard {

template <typename F>
class ScopeGuardImpl {
public:
  explicit ScopeGuardImpl(const F &func) : _func(func) {}
  explicit ScopeGuardImpl(F && func) : _func(std::move(func)) {}
  ~ScopeGuardImpl() { _func(); }
  ScopeGuardImpl(const ScopeGuardImpl &) = delete;
  ScopeGuardImpl &operator=(const ScopeGuardImpl &) = delete;

private:
  F _func;
};

template <typename F>
static auto create(F &&f) {
  return ScopeGuardImpl<std::remove_reference_t<F>>(std::forward<F>(f));
}

}; // namespace Interject::ScopeGuard;
