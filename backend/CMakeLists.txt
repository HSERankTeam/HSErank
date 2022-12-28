cmake_minimum_required(VERSION 3.12)
project(service_template CXX)


### userver
set(USERVER_FEATURE_POSTGRESQL 0)

include(external/userver/cmake/SetupEnvironment.cmake)
include(GNUInstallDirs)

add_subdirectory(external/userver)
### ~userver

# gateway
add_executable(gateway ${CMAKE_CURRENT_SOURCE_DIR}/gatewayservice/src/main.cpp)
target_link_libraries(gateway PUBLIC userver-core)
# target_include_directories(gateway ${CMAKE_CURRENT_SOURCE_DIR}/external/userver)
# gateway