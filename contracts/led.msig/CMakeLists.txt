add_contract(led.msig led.msig ${CMAKE_CURRENT_SOURCE_DIR}/src/led.msig.cpp)

target_include_directories(led.msig
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(led.msig
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")