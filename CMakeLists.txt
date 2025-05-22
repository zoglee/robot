cmake_minimum_required(VERSION 2.6)

SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")

FIND_PACKAGE(glib REQUIRED)
FIND_PACKAGE(protobuf REQUIRED)


SET(CMAKE_USE_RELATIVE_PATHS  true )


# Define paths
SET(PROTO_FILE ${CMAKE_SOURCE_DIR}/proto/robot.proto)
SET(PROTO_OUT_DIR ${CMAKE_SOURCE_DIR}/proto/out)

# Ensure the output directory exists
FILE(MAKE_DIRECTORY ${PROTO_OUT_DIR})

# Define the generated source and header files
SET(PROTO_GENERATED_CC ${PROTO_OUT_DIR}/robot.pb.cc)
SET(PROTO_GENERATED_H ${PROTO_OUT_DIR}/robot.pb.h)

# Add a custom command to run protoc
ADD_CUSTOM_COMMAND(
    OUTPUT ${PROTO_GENERATED_CC} ${PROTO_GENERATED_H}
    COMMAND protoc --cpp_out=${PROTO_OUT_DIR} --proto_path=${CMAKE_SOURCE_DIR}/proto ${PROTO_FILE}
    DEPENDS ${PROTO_FILE}
    COMMENT "Generating C++ sources from ${PROTO_FILE}"
)

#SET(PROTO_DIR ${CMAKE_SOURCE_DIR}/proto) # Original, kept for reference
#SET(PROTO_PB_DIR ${CMAKE_SOURCE_DIR}/proto/out) # Original, kept for reference, same as PROTO_OUT_DIR
#AUX_SOURCE_DIRECTORY(${PROTO_PB_DIR} PB_SRC_LIST) # Removed as per instruction


SET(SRC_LIST 
${PROTO_GENERATED_CC}
fileutils.cc
pb_master.cc
flags.cc
config.cc
client.cc
robot.cc
main.cc
)
INCLUDE_DIRECTORIES(${PROTO_OUT_DIR}) # Was PROTO_PB_DIR, which is the same path
INCLUDE_DIRECTORIES(${GLIB_INCLUDE_DIR})


SET(BIN_NAME "robot")
ADD_DEFINITIONS( "-g -DDEBUG -Wall -MMD -funroll-loops -fPIC" )
ADD_EXECUTABLE(${BIN_NAME} ${SRC_LIST})
TARGET_LINK_LIBRARIES (${BIN_NAME} ${GLIB_LIBRARY} "ssl" "glog" "gflags" "protobuf" "pthread")
