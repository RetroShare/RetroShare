# ------------------------------------------------------------------------ *\
# CMakeLists.txt
# This file is part of pegmarkdown
#
# Copyright (C) 2026      David Bears <dbear4q@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
# ------------------------------------------------------------------------ */

cmake_minimum_required(VERSION 3.24...4.4)

project(pegmarkdown
  LANGUAGES C
)

add_library(pegmarkdown STATIC
  GLibFacade.h
  markdown_lib.h
	markdown_peg.h
	odf.h
	parsing_functions.h
	utility_functions.h

  GLibFacade.c
	markdown_lib.c
  markdown_output.c
	markdown_parser.c
	parsing_functions.c
	odf.c
	utility_functions.c
)

target_include_directories(pegmarkdown PUBLIC ${PROJECT_SOURCE_DIR})

add_library(pegmarkdown::pegmarkdown ALIAS pegmarkdown)
