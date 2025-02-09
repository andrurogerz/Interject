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

#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include "symbols.hxx"

namespace Interject {

class Transaction {
public:
  enum ResultCode {
    Success = 0,
    ErrorNotImplemented,
    ErrorInvalidState,
    ErrorSymbolNotFound,
    ErrorUnexpected,
    ErrorMProtectFailure,
  };

  class Builder {
  public:
    template <typename T>
    Builder &add(const std::string_view &name, T *hook, T **trampoline) {
      names.emplace_back(name);
      hook_addrs.emplace_back(reinterpret_cast<std::uintptr_t>(hook));
      trampoline_addrs.emplace_back(reinterpret_cast<std::uintptr_t*>(trampoline));
      return *this;
    }

    Transaction build() const {
      return Transaction(std::move(names), std::move(hook_addrs));
    }

  private:
    std::vector<std::string_view> names;
    std::vector<std::uintptr_t> hook_addrs;
    std::vector<std::uintptr_t*> trampoline_addrs;
  };

  ResultCode prepare();

  ResultCode abort();

  ResultCode commit();

private:
  enum State {
    TxnInitialized = 0,
    TxnPrepared,
    TxnAborted,
    TxnCommitted,
  };

  Transaction(const std::vector<std::string_view> &&names,
              const std::vector<std::uintptr_t> hook_addrs)
      : _state(TxnInitialized), _names(std::move(names)),
        _hook_addrs(std::move(hook_addrs)) {}

  static std::size_t _pageSize;

  State _state;
  std::vector<std::string_view> _names;
  std::vector<std::uintptr_t> _hook_addrs;
  std::vector<Symbols::Descriptor> _descriptors;
  std::unordered_map<uintptr_t, int> _pagePermissions;
};

}; // namespace Interject
