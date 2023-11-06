cmake_minimum_required(VERSION 3.19.0)

# Install the engine
install(TARGETS montezuma)

# Package it
set(CPACK_PACKAGE_VENDOR "Michele Bolognini")
if (WINDOWS)
    set(CPACK_GENERATOR "NSIS")
else()
    set(CPACK_GENERATOR "DEB")
endif()
set(CPACK_THREADS 0)
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Michele Bolognini")
include(CPack)
