message(STATUS "FetchContent: civetweb")

# Disable builds
set(CIVETWEB_BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(CIVETWEB_ENABLE_SERVER_EXECUTABLE OFF CACHE BOOL "" FORCE)

# Disable ASAN debug
set(CIVETWEB_ENABLE_ASAN OFF CACHE BOOL "" FORCE)

# Enable websocket connections
set(CIVETWEB_ENABLE_WEBSOCKETS ON CACHE BOOL "" FORCE)

# Disable IPv6 as we use only IPv4
set(CIVETWEB_ENABLE_IPV6 OFF CACHE BOOL "" FORCE)

if (APPLE)
    add_compile_options(
        -Wno-expansion-to-defined
        -Wno-extra-semi
        -Wno-macro-redefined
        -Wno-nullability-completeness
        -Wno-nullability-extension
        -Wno-used-but-marked-unused
    )
endif ()

# Civetweb v1.15 already contains the local patch changes that were
# historically applied here. Set a minimum policy version only while
# adding the third-party project so CMake 4.x accepts its older policy
# declarations without changing the fetched source.
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

FetchContent_Declare(
    civetweb
    GIT_REPOSITORY https://github.com/civetweb/civetweb.git
    GIT_TAG v1.15
)
FetchContent_MakeAvailable(civetweb)
unset(CMAKE_POLICY_VERSION_MINIMUM)

set_target_properties(civetweb-c-library PROPERTIES FOLDER contrib)

target_compile_options(civetweb-c-library PUBLIC -DUSE_IPV6=1)
