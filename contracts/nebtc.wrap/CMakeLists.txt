add_contract(nebtc.wrap nebtc.wrap ${CMAKE_CURRENT_SOURCE_DIR}/src/nebtc.wrap.cpp)

target_include_directories(nebtc.wrap
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(nebtc.wrap
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")