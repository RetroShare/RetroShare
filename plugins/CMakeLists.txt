# plugins/CMakeLists.txt
cmake_minimum_required(VERSION 3.18)

# Per-plugin toggles: allow building a subset of plugins. Useful when one plugin
# is mid-port (e.g. to Qt6) and would otherwise break the whole build — disable
# it with -DRS_PLUGIN_<NAME>=OFF and keep compiling the rest.
option(RS_PLUGIN_FEEDREADER "Build the FeedReader plugin" ON)
option(RS_PLUGIN_VOIP       "Build the VOIP plugin"       ON)

if(RS_GUI)
    if(RS_PLUGIN_FEEDREADER)
        add_subdirectory(FeedReader)
    endif()
    if(RS_PLUGIN_VOIP)
        add_subdirectory(VOIP)
    endif()
endif()
