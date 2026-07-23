# RetroShare decentralized communication platform
#
# SPDX-License-Identifier: CC0-1.0
#
# Minimal CMake build for the bundled peg-markdown static library, used by the
# WikiPoos GUI (retroshare-gui, guarded by RS_GXSWIKIPOS). The historical qmake
# build (pegmarkdown.pro) linked a pre-built libpegmarkdown.a; this reproduces
# it as a normal CMake target.
#
# We use the bundled "dummy glib" (GLibFacade + the local glib.h) on every
# platform, exactly like the Windows/macOS qmake builds did, so there is no
# dependency on system glib-2.0.

cmake_minimum_required(VERSION 3.5)
project(pegmarkdown C)

add_library(pegmarkdown STATIC
	markdown_lib.c
	markdown_parser.c
	parsing_functions.c
	markdown_output.c
	odf.c
	utility_functions.c
	GLibFacade.c )

# Some of the (old) sources rely on GNU/POSIX libc extensions (e.g. strdup).
target_compile_definitions(pegmarkdown PRIVATE _GNU_SOURCE)

# Consumers include "markdown_lib.h" and, transitively, the bundled dummy
# <glib.h> from this same directory.
target_include_directories(pegmarkdown PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Third-party legacy C: silence its warnings so they neither clutter the build
# nor break it under a strict toolchain.
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
	target_compile_options(pegmarkdown PRIVATE -w)
endif()
