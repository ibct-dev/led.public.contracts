add_contract(led.token led.token ${CMAKE_CURRENT_SOURCE_DIR}/src/led.token.cpp)

target_include_directories(led.token
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(led.token
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
