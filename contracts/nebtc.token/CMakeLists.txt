add_contract(nebtc.token nebtc.token ${CMAKE_CURRENT_SOURCE_DIR}/src/nebtc.token.cpp)

target_include_directories(nebtc.token
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(nebtc.token
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")
