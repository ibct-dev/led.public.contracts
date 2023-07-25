add_contract(led.system led.system
   ${CMAKE_CURRENT_SOURCE_DIR}/src/nebtc.system.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/delegate_bandwidth.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/exchange_state.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/native.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/producer_pay.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/producer.cpp
)

target_include_directories(nebtc.system
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../nebtc.token/include)

set_target_properties(nebtc.system
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")