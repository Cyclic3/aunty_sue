cmake_minimum_required(VERSION 3.10)

find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND AND NOT DISABLE_CCACHE)
  message("Using ccache")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
else()
  message("Not using ccache")
endif()

project(aunty_sue VERSION 0.0.1)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

file(GLOB_RECURSE ${PROJECT_NAME}_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES})

# Begin requirements
find_package(Threads REQUIRED)
#find_package(TBB REQUIRED)
find_package(Boost REQUIRED)
# End requirements

target_link_libraries(${PROJECT_NAME}
PUBLIC # Libraries needed to use this library
#TBB::tbb
Boost::boost
Threads::Threads
PRIVATE # Libraries only needed to build this library
# @@
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_17)

include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
