# protobuf_generate is provided by find_package(protobuf CONFIG) automatically.
# When using FetchContent, we load the generate script from the source tree.
if(DEFINED protobuf_SOURCE_DIR)
  include(${protobuf_SOURCE_DIR}/cmake/protobuf-generate.cmake)
endif()

set(PROTO_FILES
  ibridger/envelope.proto
  ibridger/rpc.proto
  ibridger/examples/echo.proto
)

add_library(ibridger_proto STATIC ${PROTO_FILES})

target_link_libraries(ibridger_proto PUBLIC protobuf::libprotobuf)

protobuf_generate(
  TARGET      ibridger_proto
  LANGUAGE    cpp
  IMPORT_DIRS ${CMAKE_CURRENT_SOURCE_DIR}
)

# Generated headers land in the build tree; expose that directory.
target_include_directories(ibridger_proto PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
)
