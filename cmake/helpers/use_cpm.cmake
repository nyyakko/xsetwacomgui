set(CPM_VERSION v0.42.0)

file(
    DOWNLOAD
    https://github.com/cpm-cmake/CPM.cmake/releases/download/${CPM_VERSION}/CPM.cmake
    ${CMAKE_SOURCE_DIR}/cmake/CPM.cmake
)

include(${CMAKE_SOURCE_DIR}/cmake/CPM.cmake)

