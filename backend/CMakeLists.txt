 cmake_minimum_required(VERSION 3.12)
project(service_template CXX)


# set(CMAKE_CXX_STANDARD 20)

### userver
include(external/userver/cmake/SetupEnvironment.cmake)
include(GNUInstallDirs)
add_subdirectory(external/userver)
### ~userver

# gateway
add_executable(gateway ${CMAKE_CURRENT_SOURCE_DIR}/gatewayservice/src/main.cpp)
target_link_libraries(gateway PUBLIC userver-core)
set_target_properties(gateway PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/gateway
)
file(
    COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/gatewayservice/src/static_config.yaml
    DESTINATION
    ${CMAKE_CURRENT_SOURCE_DIR}/bin/gateway/
)
file(
    COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/gatewayservice/src/dynamic_config.json
    DESTINATION
    ${CMAKE_CURRENT_SOURCE_DIR}/bin/gateway/
)
# gateway