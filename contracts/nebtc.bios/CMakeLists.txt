add_contract(led.bios led.bios ${CMAKE_CURRENT_SOURCE_DIR}/src/led.bios.cpp)

target_include_directories(nebtc.bios
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(nebtc.bios
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")