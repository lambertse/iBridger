#pragma once

#include <memory>
#include <system_error>
#include <utility>

#include "ibridger/envelope.pb.h"
#include "ibridger/protocol/framing.h"

namespace ibridger {
namespace protocol {

/// Serializes and deserializes protobuf `Envelope` messages over a
/// `FramedConnection`.
///
/// Owns a shared_ptr<FramedConnection> so that both the read path and the
/// write path inside a single connection handler can share the same framed
/// connection.
///
/// Error codes returned:
///   - std::errc::bad_message  — protobuf parse failure (corrupted frame)
///   - any error from FramedConnection — transport / framing failure
class EnvelopeCodec {
 public:
  explicit EnvelopeCodec(std::shared_ptr<FramedConnection> conn);

  /// Serialize `envelope` and send it as a single frame.
  std::error_code send(const ibridger::Envelope& envelope);

  /// Receive the next frame and deserialize it into an Envelope.
  /// Returns (Envelope{}, error) on any failure.
  std::pair<ibridger::Envelope, std::error_code> recv();

 private:
  std::shared_ptr<FramedConnection> conn_;
};

}  // namespace protocol
}  // namespace ibridger
