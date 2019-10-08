add_contract(led.wrap led.wrap ${CMAKE_CURRENT_SOURCE_DIR}/src/led.wrap.cpp)

target_include_directories(led.wrap
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(led.wrap
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")