cmake_minimum_required(VERSION 3.5)
project(vgui_support)

set(VGUI_SUPPORT_LIB vgui_support)

file(GLOB_RECURSE VGUI_SUPPORT_SRC "vgui_support/*.cpp")

include_directories(
	"../common"
	"../dlls"
	"../public"
	"../engine"
	"vgui_support/vgui-dev/include"
)

add_definitions(-DINTERNAL_VGUI_SUPPORT -DSTDINT_H="stdint.h")

add_library(${VGUI_SUPPORT_LIB} OBJECT ${VGUI_SUPPORT_SRC})