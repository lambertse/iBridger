#pragma once

// iBridger core — convenience header.
// Including this single header gives access to the full core API.

#include "ibridger/common/error.h"
#include "ibridger/common/logger.h"
#include "ibridger/protocol/envelope_codec.h"
#include "ibridger/protocol/framing.h"
#include "ibridger/rpc/builtin/ping_service.h"
#include "ibridger/rpc/client.h"
#include "ibridger/rpc/server.h"
#include "ibridger/rpc/service.h"
#include "ibridger/transport/connection.h"
#include "ibridger/transport/transport.h"
#include "ibridger/transport/transport_factory.h"
