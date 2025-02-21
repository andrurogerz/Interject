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

#include "transaction.hxx"
#include "disassembler.hxx"
#include "memory_map.hxx"
#include "patch.hxx"
#include "scope_guard.hxx"
#include "signal_action.hxx"
#include "symbols.hxx"
#include "threads.hxx"
#include "unwind.hxx"

#include <cstdint>
#include <cstring>
#include <format>
#include <iostream>

#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>

namespace Interject {

std::size_t Transaction::_pageSize =
    static_cast<std::size_t>(sysconf(_SC_PAGESIZE));

Transaction::ResultCode Transaction::prepare() {
  if (_state != TxnInitialized) {
    return ErrorInvalidState;
  }

  std::vector<Symbols::Descriptor> descriptors(_names.size());
  Symbols::lookup(_names, descriptors);

  MemoryMap map;
  if (!map.load()) {
    std::cerr << "Failed loading memory map\n";
    return ErrorUnexpected;
  }

  std::unordered_map<uintptr_t, int> pagePermissions;
  std::vector<std::vector<uint8_t>> origInstrs;

  for (size_t idx = 0; idx < _names.size(); idx++) {
    auto descriptor = descriptors[idx];

    if (Patch::jumpToSize() > descriptor.size) {
      std::cerr << std::format("function {} too small to patch ({} < {} bytes)\n", _names[idx], descriptor.size, Patch::jumpToSize());
      return ErrorFunctionBodyTooSmall;
    }

    const auto addr = descriptor.addr;
    if (addr == 0) {
      return ErrorSymbolNotFound;
    }

    auto instrs = Disassembler::copyInstrs(descriptor.addr, descriptor.size,
                                           Patch::jumpToSize());
    if (!instrs) {
      std::cerr << "Failed disassembling function\n";
      return ErrorUnexpected;
    }

    origInstrs.emplace_back(std::vector<uint8_t>(instrs->begin(), instrs->end()));

    const auto pageMask = ~(_pageSize - 1);
    const auto firstPageAddr = addr & pageMask;
    const auto lastPageAddr = (addr + descriptor.size - 1) & pageMask;

    for (auto pageAddr = firstPageAddr; pageAddr <= lastPageAddr; pageAddr += _pageSize) {
      if (pagePermissions.find(pageAddr) != pagePermissions.end()) {
        continue;
      }

      const auto region = map.find(pageAddr);
      if (!region) {
        return ErrorSymbolNotFound;
      }

      pagePermissions[pageAddr] = region->permissions;
    }

    // TODO: allocate trampoline and copy minimal instruction sequence from
    // the target function to the trampoline location.
  }

  _state = TxnPrepared;
  _descriptors = std::move(descriptors);
  _pagePermissions = std::move(pagePermissions);
  _origInstrs = std::move(origInstrs);
  return Success;
}

void Transaction::backtraceHandler(int signal, siginfo_t *info,
                                   void *context) noexcept {
  const pid_t tid = ::gettid();
  if (info->si_signo != SIGUSR1) {
    // Received an unexpected signal. This should never happen.
    return;
  }

  ThreadControlBlock *controlBlock =
      reinterpret_cast<ThreadControlBlock *>(info->si_value.sival_ptr);
  const pid_t targetTid = __atomic_load_n(&controlBlock->tid, __ATOMIC_ACQUIRE);
  __atomic_store_n(&controlBlock->tid, tid, __ATOMIC_RELEASE);
  if (targetTid != tid) {
    // This signal handler is running on a different thread than the signaller
    // expected. It might even be running on the signalling thread itself. In
    // this case, immediately signal completion after memoizing our actual tid
    // and don't block before returning. The signaller is responsible for
    // checking the memoized tid against the tid it expected to run the handler
    // and to retry if necessary.
    controlBlock->handlerWork.Set();
    return;
  }

  const size_t frameCount = Unwind::backtrace(std::span(controlBlock->frames));
  for (size_t idx = 0; idx < frameCount; idx++) {
    // Force memory barrier on all of the stack frames to ensure they can be
    // properly ready by the signaller.
    const auto frame = controlBlock->frames[idx];
    __atomic_store_n(&controlBlock->frames[idx], frame, __ATOMIC_RELEASE);
  }
  __atomic_store_n(&controlBlock->frameCount, frameCount, __ATOMIC_RELEASE);
  controlBlock->handlerExit.Reset();
  controlBlock->handlerWork
      .Set(); // let the signaller know we're done capturing backtrace
  controlBlock->handlerExit
      .Wait(); // wait for the signaller to release us before returning
}

Transaction::ResultCode Transaction::preparePagesForWrite() const noexcept {
  for (const auto &pair : _pagePermissions) {
    const int prot = pair.second | PROT_WRITE;
    if (::mprotect(reinterpret_cast<void *>(pair.first), _pageSize, prot) !=
        0) {
      std::cerr << "failed setting PROT_EXEC on page starting at 0x" << std::hex
                << pair.first << std::endl;
      return ErrorMemoryProtectionFailure;
    }
  }
  return Success;
}

Transaction::ResultCode Transaction::restorePagePermissions() const noexcept {
  for (const auto &pair : _pagePermissions) {
    if (::mprotect(reinterpret_cast<void *>(pair.first), _pageSize,
                   pair.second) != 0) {
      std::cerr << "failed to restore permissions on page starting at 0x"
                << std::hex << pair.first << std::endl;
      return ErrorMemoryProtectionFailure;
    }
  }
  return Success;
}

bool Transaction::isPatchTarget(std::uintptr_t addr) const noexcept {
  const std::size_t patchSize = Patch::jumpToSize();
  for (auto &descriptor : _descriptors) {
    if (addr < descriptor.addr || addr >= descriptor.addr + patchSize) {
      continue;
    }
    return true;
  }
  return false;
}

Transaction::ResultCode Transaction::haltThread(
    pid_t targetTid,
    Transaction::ThreadControlBlock &controlBlock) const noexcept {
  size_t retryWaitUs = 1;
  struct timespec timeout = {.tv_sec = 1};

  SignalAction action(SIGUSR1, backtraceHandler, SA_SIGINFO);
  if (action.failed()) {
    return ErrorSignalActionFailure;
  }

  const union sigval sigval = {.sival_ptr = &controlBlock};

  for (;;) {
    __atomic_store_n(&controlBlock.tid, targetTid, __ATOMIC_RELEASE);
    controlBlock.handlerWork.Reset();

    if (::sigqueue(targetTid, SIGUSR1, sigval) == -1) {
      std::cerr << std::format("failed to signal tid:{:#x} errno:{} ({})\n",
                               targetTid, errno, ::strerror(errno));
      return ErrorSignalActionFailure;
    }

    if (!controlBlock.handlerWork.Wait(&timeout)) {
      std::cerr << std::format(
          "timed out waiting for tid:{:#x} to be signalled\n", targetTid);
      return ErrorTimedOut;
    }

    const pid_t actualTid =
        __atomic_load_n(&controlBlock.tid, __ATOMIC_ACQUIRE);

    // If the handler ran on a different thread than we expected (most likely
    // on this thread), the handler exited without capturing a backtrace. In
    // this situation we need to retry.
    bool needRetry = actualTid != targetTid;
    for (size_t jdx = 0; !needRetry && jdx < controlBlock.frameCount; jdx++) {
      const auto frame =
          __atomic_load_n(&controlBlock.frames[jdx], __ATOMIC_ACQUIRE);
      needRetry = isPatchTarget(reinterpret_cast<uintptr_t>(frame));
    }

    if (!needRetry) {
      // The thread is not executing in a target instruction sequence and is
      // halted in the signal handler.
      break;
    }

    // Let the handler exit immediately so we can signal it again and try to
    // capture it executing in a different location.
    controlBlock.handlerExit.Set();

    // Expontntial backoff (up to 1s) on retry to give the handler a chance
    // to exit and the thread a chance to run past the patch target
    // instructions.
    if (retryWaitUs > 1000000) {
      return ErrorTimedOut;
    }

    ::usleep(retryWaitUs);
    retryWaitUs <<= 1;
  }

  return Success;
}

Transaction::ResultCode Transaction::patch(PatchCommand command) {
  // First, prepare the first page of every hooked function to be writable so
  // we can overwrite the first few instructions with a jump to the replacement
  // function.
  ResultCode result = preparePagesForWrite();
  if (result != Success) {
    return result;
  }

  // Ensure the original page permissions are unconditionally restored on
  // success or failure.
  const auto pagePermissionsGuard =
      ScopeGuard::create([&]() { (void)restorePagePermissions(); });

  // We have to do a bit of a complex dance when patching the target instruction
  // sequence with a new instruction sequence. The primary issue is that any
  // other thread in the process may be concurrently executing the target
  // instruction sequence, and modifying it mid-execution will result in
  // undefined behavior.
  //
  // The solution is to interrupt every other thread in the process with a
  // user-defined signal and custom signal handler. The signal handler captures
  // a backtrace which is used to determine if the thread is executing within a
  // target instruction sequence. The handler then prevents the thread from
  // resuming until patches are applied. In the (unlikely) case that a thread IS
  // concurrently executing in a target instruction sequence, we exit the signal
  // handler to resume thread execution, sleep briefly, and try again.

  // TODO: there is still a race condition where threads created after capturing
  // this thread snapshot will not be accounted for. While unlikely to cause a
  // problem in practice, we need to handle this case to be completely correct.
  std::optional<std::vector<pid_t>> threadSnapshot = Threads::all();
  if (!threadSnapshot) {
    std::cerr << "failed enumerating all threads" << std::endl;
    return ErrorUnexpected;
  }

  // Now that we know how many threads we have, dynamically allocate a vector
  // of control structures, one for each thread. We preallocate here to avoid
  // heap allocations while halting threads.
  std::vector<ThreadControlBlock> threadControlBlocks(threadSnapshot->size());

  // Unconditionally set all thread exit events on success or failure to ensure
  // any interruped threads exit their signal handlers and resume.
  const auto threadReleaseGuard = ScopeGuard::create([&]() {
    for (auto &controlBlock : threadControlBlocks) {
      controlBlock.handlerExit.Set();
    }
  });

  // Iterate over the thread list halting each thread and ensuring it is not
  // executing a target instruction sequence.
  //
  // Once we start halting threads, we need to take care to not perform any
  // operation that could deadlock. For example, a thread could be halted mid
  // resource acquisition while holding a mutex. If we attempt an operation that
  // acquires the same mutex on the current thread, we'll deadlock. Allocating
  // from the heap is one such example so we even avoid heap allocations.
  for (size_t idx = 0; idx < threadSnapshot->size(); idx++) {
    const pid_t targetTid = (*threadSnapshot)[idx];
    if (targetTid == ::gettid()) {
      // Skip the current thread to avoid deadlock. We know it isn't executing
      // the target code so this is fine.
      continue;
    }

    result = haltThread(targetTid, threadControlBlocks[idx]);
    if (result != Success) {
      return result;
    }
  }

  // Patch each function with jump to hook location.
  for (size_t idx = 0; idx < _descriptors.size(); idx++) {
    const auto &descriptor = _descriptors[idx];
    const auto targetAddr = reinterpret_cast<char*>(descriptor.addr);
    const uintptr_t hookAddr = _hooks[idx];

    if (command == Apply) {
      // Generate an instruction sequence that jumps to the hook address to
      // patch over the existing function.
      auto instrBytes = Patch::createJumpTo(hookAddr);

      std::memcpy(targetAddr, instrBytes.data(), instrBytes.size());

      // Flush the instruction cache after patching.
      __builtin___clear_cache(targetAddr, targetAddr + instrBytes.size());

    } else if (command == Restore) {
      auto &instrBytes = _origInstrs[idx];
      std::memcpy(targetAddr, instrBytes.data(), instrBytes.size());

      // Flush the instruction cache after patching.
      __builtin___clear_cache(targetAddr, targetAddr + instrBytes.size());
    }
  }
  return Success;
}

Transaction::ResultCode Transaction::commit() {
  return patch(Apply);
}

Transaction::ResultCode Transaction::rollback() {
  return patch(Restore);
}

}; // namespace Interject
