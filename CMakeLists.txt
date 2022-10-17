project(netron)

include(FetchContent)
FetchContent_Declare(
  asio
  GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
  GIT_TAG 147f7225a96d45a2807a64e443177f621844e51c
)
FetchContent_MakeAvailable(asio)

file(GLOB_RECURSE netron-sources "include/*.hpp")
add_library(netron INTERFACE)
target_sources(netron INTERFACE ${netron-sources})
target_precompile_headers(netron INTERFACE "include/netron/common.hpp")
target_include_directories(netron 
  INTERFACE include
  INTERFACE ${asio_SOURCE_DIR}/asio/include
)

file(GLOB netron-examples "examples/*.cpp")
foreach(example ${netron-examples})
  get_filename_component(example-name ${example} NAME_WE)
  add_executable(${example-name} ${example})
  target_link_libraries(${example-name} netron)
endforeach()
