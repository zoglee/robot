FIND_PATH( PROTOBUF_INCLUDE_DIR message.h
	/usr/local/include/google/protobuf
	DOC "The directory where message.h resides"  )
 
SET(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
FIND_LIBRARY( PROTOBUF_LIBRARY
	NAMES protobuf
	PATHS /usr/local/lib
	DOC "The Protobuf library")

IF (PROTOBUF_INCLUDE_DIR)
	SET( PROTOBUF_FOUND 1 CACHE STRING "Set to 1 if protobuf is found, 0 otherwise")
ELSE (PROTOBUF_INCLUDE_DIR)
	SET( PROTOBUF_FOUND 0 CACHE STRING "Set to 1 if protobuf is found, 0 otherwise")
ENDIF (PROTOBUF_INCLUDE_DIR)

MARK_AS_ADVANCED( PROTOBUF_FOUND )

IF(PROTOBUF_FOUND)
	MESSAGE(STATUS "找到了 protobuf 库")
ELSE(PROTOBUF_FOUND)
	MESSAGE(FATAL_ERROR "没有找到 protobuf 库 :请安装它, 建议源码安装")
ENDIF(PROTOBUF_FOUND)


