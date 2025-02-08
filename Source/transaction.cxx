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
#include "symbols.hxx"

#include <format>
#include <iostream>

namespace Interject {

Transaction::ResultCode Transaction::prepare() {
  if (_state != TxnInitialized) {
    return ErrorInvalidState;
  }

  std::vector<Symbols::Descriptor> descriptors(_names.size());
  Symbols::lookup(_names, descriptors);

  for (size_t idx = 0; idx < _names.size(); idx++) {
    if (descriptors[idx].addr == 0) {
      return ErrorSymbolNotFound;
    }

    // TODO: allocate trampoline and copy function preamble to trampoline
    // location
  }

  _state = TxnPrepared;
  _descriptors = std::move(descriptors);
  return Success;
}

Transaction::ResultCode Transaction::commit() {
  // TODO: prepare function memory for write

  // TODO: patch each function with jump to hook location

  // TODO: restore original memory protection

  return ErrorNotImplemented;
}

Transaction::ResultCode Transaction::abort() {

  // TODO: restore original memory protection

  return Success;
}

}; // namespace Interject
