add_contract(led.system led.system
   ${CMAKE_CURRENT_SOURCE_DIR}/src/led.system.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/delegate_bandwidth.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/exchange_state.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/native.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/producer_pay.cpp
   ${CMAKE_CURRENT_SOURCE_DIR}/src/producer.cpp
)

target_include_directories(led.system
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../led.token/include)

set_target_properties(led.system
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")