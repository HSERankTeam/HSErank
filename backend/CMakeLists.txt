cmake_minimum_required(VERSION 3.12)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(service_template CXX)

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

# dblp
find_package(ZLIB REQUIRED)
add_executable(dblp 
    ${CMAKE_CURRENT_SOURCE_DIR}/dblpservice/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/dblpservice/src/xml_dblp.h
)
target_link_libraries(dblp PUBLIC userver-core userver-postgresql ZLIB::ZLIB)
set_target_properties(dblp PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/dblp
)
file(
    COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/dblpservice/src/static_config.yaml
    DESTINATION
    ${CMAKE_CURRENT_SOURCE_DIR}/bin/dblp/
)
file(
    COPY
    ${CMAKE_CURRENT_SOURCE_DIR}/dblpservice/src/dynamic_config.json
    DESTINATION
    ${CMAKE_CURRENT_SOURCE_DIR}/bin/dblp/
)
# dblp