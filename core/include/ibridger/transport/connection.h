#pragma once

#include <cstddef>
#include <cstdint>
#include <system_error>
#include <utility>

#include "ibridger/transport/types.h"

namespace ibridger {
namespace transport {

class IConnection {
 public:
  virtual ~IConnection() = default;

  /// Send up to `len` bytes from `data`. Returns (bytes_sent, error).
  /// May send fewer than `len` bytes without error (partial write).
  virtual std::pair<size_t, std::error_code> send(const uint8_t* data,
                                                  size_t len) = 0;

  /// Receive up to `len` bytes into `buf`. Returns (bytes_received, error).
  /// Returns (0, {}) on clean EOF / peer disconnect.
  virtual std::pair<size_t, std::error_code> recv(uint8_t* buf, size_t len) = 0;

  /// Close the connection. Safe to call multiple times.
  virtual void close() = 0;

  /// True if the connection is open and usable.
  virtual bool is_connected() const = 0;

  /// Unique identifier for this connection instance.
  virtual ConnectionId id() const = 0;
};

}  // namespace transport
}  // namespace ibridger
