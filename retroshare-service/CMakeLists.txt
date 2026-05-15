################################################################################
# retroshare-service/CMakeLists.txt
################################################################################

# project() and common options are inherited from the root CMakeLists.txt

# --- Sources ---
set(RS_SERVICE_SOURCES src/retroshare-service.cc)

# --- Executable ---
add_executable(retroshare-service ${RS_SERVICE_SOURCES})
target_compile_definitions(retroshare-service PRIVATE RS_RELEASE_VERSION="${RS_VERSION}")

# --- Linkage ---
# Linked to the retroshare library target defined in libretroshare/
target_link_libraries(retroshare-service PRIVATE retroshare)

# --- Compile Definitions ---
if(RS_JSON_API)
    target_compile_definitions(retroshare-service PUBLIC RS_JSONAPI)
endif()

if(RS_SERVICE_TERMINAL_LOGIN)
    target_compile_definitions(retroshare-service PUBLIC RS_SERVICE_TERMINAL_LOGIN)
endif()

if(RS_WEBUI)
    target_compile_definitions(retroshare-service PUBLIC RS_WEBUI)
endif()

# --- Installation ---
install(TARGETS retroshare-service RUNTIME DESTINATION bin)
