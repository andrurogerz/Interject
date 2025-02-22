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

#include "event.hxx"
#include "symbols.hxx"
#include "trampoline.hxx"

#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#include <signal.h>

namespace Interject {

class Transaction {
public:
  enum ResultCode {
    Success = 0,
    ErrorNotImplemented,
    ErrorInvalidState,
    ErrorSymbolNotFound,
    ErrorUnexpected,
    ErrorMemoryProtectionFailure,
    ErrorSignalActionFailure,
    ErrorFunctionBodyTooSmall,
    ErrorTrampolineCreationFailure,
    ErrorTimedOut,
  };

  class Builder {
  public:
    template <typename T>
    Builder &add(const std::string_view &name, T *hook, T **trampoline) {
      names.emplace_back(name);
      hooks.emplace_back(reinterpret_cast<std::uintptr_t>(hook));
      trampoline_addrs.emplace_back(
          reinterpret_cast<std::uintptr_t *>(trampoline));
      return *this;
    }

    Transaction build() const {
      return Transaction(std::move(names), std::move(hooks),
                         std::move(trampoline_addrs));
    }

  private:
    std::vector<std::string_view> names;
    std::vector<std::uintptr_t> hooks;
    std::vector<std::uintptr_t *> trampoline_addrs;
  };

  ResultCode prepare();

  ResultCode commit();

  ResultCode rollback();

private:
  enum State {
    TxnInitialized = 0,
    TxnPrepared,
    TxnCommitted,
  };

  enum PatchCommand {
    Apply = 0,
    Restore = 1,
  };

  struct ThreadControlBlock {
    static constexpr size_t MAX_FRAME_COUNT = 64;
    pid_t tid;
    Event handlerWork;
    Event handlerExit;
    size_t frameCount;
    void *frames[MAX_FRAME_COUNT];
  };

  Transaction(const std::vector<std::string_view> &&names,
              const std::vector<std::uintptr_t> &&hooks,
              const std::vector<std::uintptr_t *> &&trampolineAddrs) noexcept
      : _state(TxnInitialized), _names(std::move(names)),
        _hooks(std::move(hooks)), _trampolineAddrs(trampolineAddrs) {}

  Transaction(const Transaction &) = delete;
  Transaction &operator=(const Transaction &) = delete;

  Transaction(Transaction &&other) noexcept
      : _state(other._state), _names(std::move(other._names)),
        _hooks(std::move(other._hooks)),
        _descriptors(std::move(other._descriptors)),
        _trampolineAddrs(std::move(other._trampolineAddrs)),
        _trampolines(std::move(other._trampolines)),
        _pagePermissions(std::move(other._pagePermissions)) {}

  Transaction &operator=(Transaction &&other) noexcept {
    if (this != &other) {
      _state = other._state;
      _names = std::move(other._names);
      _descriptors = std::move(other._descriptors);
      _trampolineAddrs = std::move(other._trampolineAddrs);
      _trampolines = std::move(other._trampolines);
      _pagePermissions = std::move(other._pagePermissions);
    }
    return *this;
  }

  [[nodiscard]]
  bool isPatchTarget(std::uintptr_t addr) const noexcept;

  [[nodiscard]]
  ResultCode preparePagesForWrite() const noexcept;

  [[nodiscard]]
  ResultCode restorePagePermissions() const noexcept;

  [[nodiscard]]
  ResultCode haltThread(pid_t targetTid,
                        ThreadControlBlock &controlBlock) const noexcept;

  [[nodiscard]]
  ResultCode patch(PatchCommand command);

  static void backtraceHandler(int signal, siginfo_t *info,
                               void *context) noexcept;

  static std::size_t _pageSize;

  State _state;
  std::vector<std::string_view> _names;
  std::vector<std::uintptr_t> _hooks;
  std::vector<Symbols::Descriptor> _descriptors;
  std::vector<std::uintptr_t *> _trampolineAddrs;
  std::vector<Trampoline> _trampolines;
  std::unordered_map<uintptr_t, int> _pagePermissions;
};

}; // namespace Interject
