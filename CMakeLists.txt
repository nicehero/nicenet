cmake_minimum_required(VERSION 3.14)

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
)

target_link_libraries(ggpo PUBLIC ikcp sha3 uECC winmm ws2_32)

target_sources(nicenet PRIVATE
	${SRC1}
)

add_library(nicenet STATIC)
