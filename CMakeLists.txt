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


add_library(nicenet SHARED)

add_definitions(-DASIO_STANDALONE)

if (ANDROID)
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -fcoroutines-ts")
	target_sources(nicenet PRIVATE
		${SRC1}
		${SRC2}
	)
elseif(WIN32)
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -fcoroutines")
	target_sources(nicenet PRIVATE
		${SRC1}
	)
	target_link_libraries(nicenet PUBLIC ikcp sha3 uECC winmm ws2_32 mswsock)
else()
	#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++20 -fcoroutines")
	target_sources(nicenet PRIVATE
		${SRC1}
	)
	target_link_libraries(nicenet PUBLIC ikcp sha3 uECC)
endif()

install(TARGETS nicenet DESTINATION ${PROJECT_SOURCE_DIR})
