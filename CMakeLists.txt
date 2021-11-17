cmake_minimum_required(VERSION 3.1)

# TODO: SDK Versioning.
project(nicenet VERSION 1.0.0)
set(SRC1
	"Clock.cpp"
	"HttpServer.cpp"
	"Kcp.cpp"
	"Log.cpp"
	"Server.cpp"
	"Service.cpp"
	"Tcp.cpp"
)

LINK_DIRECTORIES(
	${PROJECT_SOURCE_DIR}/dep/lib
	)

INCLUDE_DIRECTORIES(
	${PROJECT_SOURCE_DIR}/dep/include
	${PROJECT_SOURCE_DIR}/dep/include/asio
)

set(SRC2
	"dep/include/micro-ecc/uECC.c"
	"dep/include/tiny_sha3/sha3.c"
	"dep/include/kcp/ikcp.c"
	"jni.cpp"
)



option(CORO "option for coro" OFF)
add_definitions(-DASIO_STANDALONE)
set(CORO_END "")
if (CORO)
	set(CORO_END "Coro")
endif()
set(oname nicenet${CORO_END})
if (ANDROID)
	if (CORO)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -fcoroutines-ts")
	endif()
	set(ALL_SRC
		${SRC1}
		${SRC2}
	)
	add_library(${oname} SHARED ${ALL_SRC})
elseif(WIN32)
	if (CORO)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -fcoroutines")
	endif()
	set(ALL_SRC
		${SRC1}
	)
	add_library(${oname} SHARED ${ALL_SRC})
	target_link_libraries(${oname} PUBLIC ikcp sha3 uECC winmm ws2_32 mswsock)
else()
	if (CORO)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -fcoroutines")
	endif()
	set(ALL_SRC
		${SRC1}
	)
	add_library(${oname} SHARED ${ALL_SRC})
	target_link_libraries(${oname} PUBLIC ikcp sha3 uECC)
endif()


FILE(GLOB INSTALL_HEAD_FILES ${PROJECT_SOURCE_DIR}/*.h ${PROJECT_SOURCE_DIR}/*.hpp)
FILE(GLOB INSTALL_PROTO_FILES "${PROJECT_SOURCE_DIR}/proto/*")
install(TARGETS ${oname} DESTINATION ${PROJECT_SOURCE_DIR})
install(TARGETS ${oname} DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(FILES ${INSTALL_HEAD_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/nicenet)
install(FILES ${INSTALL_PROTO_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/proto)
