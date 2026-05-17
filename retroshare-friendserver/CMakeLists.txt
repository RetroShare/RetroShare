# ##############################################################################
# retroshare-friendserver/CMakeLists.txt
# ##############################################################################

# project() and common options are inherited from the root CMakeLists.txt

# --- Sources ---
set(RS_FRIENDSERVER_SOURCES
    src/retroshare-friendserver.cc
    src/friendserver.cc
    src/network.cc
)

# --- Executable ---
add_executable(retroshare-friendserver ${RS_FRIENDSERVER_SOURCES})

# --- Linkage ---
# Linked to the retroshare library target defined in libretroshare/
target_link_libraries(retroshare-friendserver PRIVATE retroshare)

# --- Includes ---
target_include_directories(retroshare-friendserver PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/libretroshare/src
)

# --- Development Debug Flags ---
if(RS_DEVELOPMENT_BUILD)
    target_compile_options(retroshare-friendserver PRIVATE "-g" "-O0")
endif()

# --- Installation ---
install(TARGETS retroshare-friendserver RUNTIME DESTINATION bin)
