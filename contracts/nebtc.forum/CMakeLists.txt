add_contract(led.forum led.forum ${CMAKE_CURRENT_SOURCE_DIR}/src/led.forum.cpp)

target_include_directories(nebtc.forum
   PUBLIC
   ${CMAKE_CURRENT_BINARY_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(nebtc.forum
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
