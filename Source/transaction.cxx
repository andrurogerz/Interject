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
#include "memory_map.hxx"
#include "patch.hxx"
#include "signal_action.hxx"
#include "symbols.hxx"
#include "threads.hxx"
#include "transaction.hxx"
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
    return ErrorUnexpected;
  }

  std::unordered_map<uintptr_t, int> pagePermissions;
  for (size_t idx = 0; idx < _names.size(); idx++) {
    auto descriptor = descriptors[idx];

    if (Patch::jumpToSize() > descriptor.size) {
      std::cerr << std::format("function {} too small to patch", _names[idx])
                << std::endl;
      return ErrorFunctionBodyTooSmall;
    }

    const auto addr = descriptor.addr;
    if (addr == 0) {
      return ErrorSymbolNotFound;
    }

    const auto page_addr = addr & ~(_pageSize - 1);
    if (pagePermissions.find(page_addr) != pagePermissions.end()) {
      // This page is already present.
      continue;
    }

    const auto region = map.find(page_addr);
    if (!region) {
      return ErrorSymbolNotFound;
    }

    pagePermissions[page_addr] = region->permissions;

    // TODO: allocate trampoline and copy function preamble to trampoline
    // location
  }

  _state = TxnPrepared;
  _descriptors = std::move(descriptors);
  _pagePermissions = std::move(pagePermissions);
  return Success;
}

constexpr size_t MAX_FRAME_COUNT = 128;
struct ThreadControl {
  pid_t tid;
  Event handlerWork;
  Event handlerExit;
  size_t stackFrameCount;
  void *stackFrames[MAX_FRAME_COUNT];
};

static void backtraceHandler(int signal, siginfo_t *info, void *context) {
  const pid_t tid = ::gettid();
  if (info->si_signo != SIGUSR1) {
    // Received an unexpected signal. This should never happen.
    return;
  }

  ThreadControl *threadControl =
      reinterpret_cast<ThreadControl *>(info->si_value.sival_ptr);
  const pid_t targetTid =
      __atomic_load_n(&threadControl->tid, __ATOMIC_ACQUIRE);
  __atomic_store_n(&threadControl->tid, tid, __ATOMIC_RELEASE);
  if (targetTid != tid) {
    // This signal handler is running on a different thread than the signaller
    // expected. It might even be running on the signalling thread itself. In
    // this case, immediately signal completion after memoizing our actual tid
    // and don't block before returning. The signaller is responsible for
    // checking the memoized tid against the tid it expected to run the handler
    // and to retry if necessary.
    threadControl->handlerWork.Set();
    return;
  }

  const size_t frameCount =
      Unwind::backtrace(std::span(threadControl->stackFrames));
  for (size_t idx = 0; idx < frameCount; idx++) {
    // Force memory barriers on all of the stack frames.
    const auto frame = threadControl->stackFrames[idx];
    __atomic_store_n(&threadControl->stackFrames[idx], frame, __ATOMIC_RELEASE);
  }
  __atomic_store_n(&threadControl->stackFrameCount, frameCount,
                   __ATOMIC_RELEASE);
  threadControl->handlerExit.Reset();
  threadControl->handlerWork
      .Set(); // let the signaller know we're done capturing backtrace
  threadControl->handlerExit
      .Wait(); // wait for the signaller to release us before returning
}

Transaction::ResultCode Transaction::preparePagesForWrite() const {
  for (const auto &pair : _pagePermissions) {
    const int prot = pair.second | PROT_WRITE;
    if (::mprotect(reinterpret_cast<void *>(pair.first), _pageSize, prot) !=
        0) {
      std::cerr << "failed setting PROT_EXEC on page starting at 0x" << std::hex
                << pair.first << std::endl;
      return ErrorMProtectFailure;
    }
  }
  return Success;
}

Transaction::ResultCode Transaction::restorePagePermissions() const {
  for (const auto &pair : _pagePermissions) {
    if (::mprotect(reinterpret_cast<void *>(pair.first), _pageSize,
                   pair.second) != 0) {
      std::cerr << "failed to restore permissions on page starting at 0x"
                << std::hex << pair.first << std::endl;
      return ErrorMProtectFailure;
    }
  }
  return Success;
}

bool Transaction::isPatchTarget(std::uintptr_t addr) const {
  const std::size_t patchSize = Patch::jumpToSize();
  for (auto &descriptor : _descriptors) {
    if (addr < descriptor.addr || addr >= descriptor.addr + patchSize) {
      continue;
    }
    return true;
  }
  return false;
}

Transaction::ResultCode Transaction::commit() {
  // First, prepare the first page of every hooked function to be writable.
  ResultCode result = preparePagesForWrite();
  if (result != Success) {
    return result;
  }

  std::optional<std::vector<pid_t>> threads;

  // We have to do a bit of a complex dance when patching the target code with
  // a new instruction sequence. The primary issue driving the complexity is
  // that any other thread in the process may be concurrently executing the
  // target instruction sequence that we intend to patch. Altering the sequence
  // mid-execution will result in undefined behavior.
  //
  // The solution is to signal every other thread in the process, determine if
  // its instruction pointer is in a patch range, and keep it from resuming
  // until all patches are applied. In the unlikely case that a thread is
  // currently executing in a patch range, we bail out, sleep briefly, and try
  // try again. We retry until all threads are suspended
  //
  // Additionally, we should handle the rare case where a thread is created
  // concurrently to the patching process. But for now we'll pretend that case
  // never occurs. Hopefully it is unlikely that any thread created concurrently
  // will not execute instructions in the patch range while we're patching. At
  // any rate, it will be tricky to test this scenario.

  std::optional<std::vector<pid_t>> tids = Threads::all();
  if (!tids) {
    std::cerr << "failed enumerating all threads" << std::endl;
    restorePagePermissions();
    return ErrorInvalidState;
  }

  std::vector<ThreadControl> threadControls(tids->size());

  for (size_t idx = 0; (result == Success) && (idx < tids->size()); idx++) {
    const pid_t targetTid = (*tids)[idx];
    if (targetTid == ::gettid()) {
      // Skip the current thread to avoid deadlock. We know it isn't executing
      // the target code so this is fine.
      continue;
    }

    size_t retryWaitUs = 1;

    struct timespec timeout = {0};
    timeout.tv_sec = 1;

    ThreadControl *threadControl = &threadControls[idx];
    SignalAction action(SIGUSR1, backtraceHandler, SA_SIGINFO);

    bool retry;
    do {
      __atomic_store_n(&threadControl->tid, targetTid, __ATOMIC_RELEASE);
      threadControl->handlerWork.Reset();

      if (::sigqueue(targetTid, SIGUSR1,
                     (union sigval){.sival_ptr = threadControl}) == -1) {
        std::cerr << std::format("failed to signal tid:{:#x} errno:{} ({})\n",
                                 targetTid, errno, ::strerror(errno));
        result = ErrorInvalidState; // TODO: better error code
        break;
      }

      if (!threadControl->handlerWork.Wait(&timeout)) {
        std::cerr << std::format(
            "timed out waiting for tid:{:#x} to be signalled\n", targetTid);
        result = ErrorInvalidState; // TODO: better error code
        break;
      }

      const pid_t actualTid =
          __atomic_load_n(&threadControl->tid, __ATOMIC_ACQUIRE);

      // If the handler ran on a different thread than we expected (most likely
      // on this thread), the handler exited without capturing a backtrace. In
      // this situation we need to retry.
      if ((retry = actualTid != targetTid)) {
        continue;
      }

      for (size_t jdx = 0; jdx < threadControl->stackFrameCount; jdx++) {
        const auto frame =
            __atomic_load_n(&threadControl->stackFrames[jdx], __ATOMIC_ACQUIRE);
        if ((retry = isPatchTarget(reinterpret_cast<uintptr_t>(frame)))) {
          break;
        }
      }

      if (retry) {
        // Let the handler exit immediately so we can signal it again and try to
        // capture it executing in a different location.
        threadControl->handlerExit.Set();

        // Expontntial backoff (up to 1s) on retry to give the handler a chance
        // to exit and the thread a chance to run past the patch target
        // instructions.
        if (retryWaitUs > 1000000) {
          result = ErrorInvalidState; // TODO: Better error code
          break;
        }

        ::usleep(retryWaitUs);
        retryWaitUs = retryWaitUs << 1;
      }
    } while (retry);
  }

  if (result == Success) {
    // Patch each function with jump to hook location.
    for (size_t idx = 0; idx < _descriptors.size(); idx++) {
      const auto &descriptor = _descriptors[idx];
      const auto targetAddr = reinterpret_cast<char *>(descriptor.addr);
      const uintptr_t hookAddr = _hooks[idx];

      auto instrBytes = Patch::createJumpTo(hookAddr);
      std::memcpy(targetAddr, &instrBytes, sizeof(instrBytes));

      // Flush the instruction cache after patching.
      __builtin___clear_cache(targetAddr, targetAddr + sizeof(instrBytes));
    }
  }

  restorePagePermissions();
  for (auto &threadControl : threadControls) {
    threadControl.handlerExit.Set();
  }

  return result;
}

Transaction::ResultCode Transaction::abort() {

  // TODO: restore original memory protection

  return Success;
}

}; // namespace Interject
