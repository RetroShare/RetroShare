# plugins/CMakeLists.txt
cmake_minimum_required(VERSION 3.24...4.4)

if(RS_GUI)
	# don't accidentally find an installed version of RetroShare-GUI
	file(WRITE ${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/RetroShare-GUIConfig.cmake
		"message(STATUS \"using RetroShare-GUI from this build\")"
	)

	add_subdirectory(FeedReader)
	add_subdirectory(VOIP)
endif()
