add_contract(nebtc.msig nebtc.msig ${CMAKE_CURRENT_SOURCE_DIR}/src/nebtc.msig.cpp)

target_include_directories(nebtc.msig
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(nebtc.msig
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")