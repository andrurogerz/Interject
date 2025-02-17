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

#include "event.hxx"
#include "symbols.hxx"

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
      return Transaction(std::move(names), std::move(hooks));
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
    TxnAborted,
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
              const std::vector<std::uintptr_t> hooks)
      : _state(TxnInitialized), _names(std::move(names)),
        _hooks(std::move(hooks)) {}

  Transaction(const Transaction&) = delete;
  Transaction &operator=(const Transaction&) = delete;

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
  std::unordered_map<uintptr_t, int> _pagePermissions;
  std::vector<std::vector<uint8_t>> _origInstrs;
};

}; // namespace Interject
