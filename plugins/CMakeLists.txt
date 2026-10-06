# plugins/CMakeLists.txt
cmake_minimum_required(VERSION 3.18...4.4)

if(RS_GUI)
    add_subdirectory(FeedReader)
    add_subdirectory(VOIP)
endif()
