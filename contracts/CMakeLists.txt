cmake_minimum_required( VERSION 3.5 )

project(contracts)

set(EOSIO_WASM_OLD_BEHAVIOR "Off")
find_package(eosio.cdt)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D${TARGET_NETWORK}")

add_subdirectory(led.bios)
add_subdirectory(led.msig)
add_subdirectory(led.wrap)
add_subdirectory(led.system)
add_subdirectory(led.token)
add_subdirectory(led.forum)
