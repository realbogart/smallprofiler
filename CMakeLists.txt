cmake_minimum_required(VERSION 3.16)
project(smallprofiler VERSION 0.1.1)

add_library(${PROJECT_NAME} INTERFACE)

target_include_directories(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_sources(${PROJECT_NAME}
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/smallprofiler.h>
        $<INSTALL_INTERFACE:include/smallprofiler.h>
)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Targets
    INCLUDES DESTINATION include
)

install(EXPORT ${PROJECT_NAME}Targets
    FILE
        ${PROJECT_NAME}Targets.cmake
    DESTINATION
        cmake
)

install(DIRECTORY include
    DESTINATION .
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(${PROJECT_NAME}ConfigVersion.cmake COMPATIBILITY SameMajorVersion)

install(
    FILES 
        cmake/${PROJECT_NAME}Config.cmake 
        ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION 
        cmake
)
