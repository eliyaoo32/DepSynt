cmake_minimum_required(VERSION 3.21)
project(reactive-synthesis-bfss)

set(CMAKE_CXX_STANDARD 17)

FILE(GLOB non_executable_SRC  ${PROJECT_SOURCE_DIR}/*.cpp ${PROJECT_SOURCE_DIR}/**/*.cpp )
list(FILTER non_executable_SRC EXCLUDE REGEX "(.*)(find_dependencies|synt_independents|synthesis).cpp")

add_executable(find_dependencies ${non_executable_SRC} find_dependencies.cpp )
add_executable(synt_independents ${non_executable_SRC} synt_independents.cpp )
add_executable(synthesis ${non_executable_SRC} synthesis.cpp )

# Include Boots Options package
find_library(boost_options_location NAMES libboost_program_options.a)
if(NOT boost_options_location)
    message(FATAL_ERROR "Boost-options library not found")
endif()
message(STATUS "Found BoostOptions location at ${boost_options_location}")
add_library(boost_options STATIC IMPORTED)
set_target_properties(boost_options PROPERTIES IMPORTED_LOCATION ${boost_options_location})

# Include Boots JSON package
find_library(boost_json_location NAMES libboost_json.a)
if(NOT boost_json_location)
    message(FATAL_ERROR "Boost-JSON library not found")
endif()
message(STATUS "Found Boost-JSON location at ${boost_json_location}")
add_library(boost_json STATIC IMPORTED)
set_target_properties(boost_json PROPERTIES IMPORTED_LOCATION ${boost_json_location})

# Include BDDX package
find_library(bddx_location NAMES libbddx.a)
if(NOT bddx_location)
    message(FATAL_ERROR "BDDX library not found")
endif()
message(STATUS "Found bddx location at ${bddx_location}")
add_library(bddx STATIC IMPORTED)
set_target_properties(bddx PROPERTIES IMPORTED_LOCATION ${bddx_location})

# Include spot package
find_library(spot_location NAMES libspot.a)
if(NOT spot_location)
    message(FATAL_ERROR "Spot library not found")
endif()
message(STATUS "Found spot location at ${spot_location}")
add_library(spot STATIC IMPORTED)
set_target_properties(spot PROPERTIES IMPORTED_LOCATION ${spot_location})

# Linking exectuables to packages
target_link_libraries( find_dependencies boost_options boost_json bddx spot )
target_link_libraries( synt_independents boost_options boost_json bddx spot )
target_link_libraries( synthesis boost_options boost_json bddx spot )

# TODO: find a way to include the spot headers and boost headers
include_directories(/usr/local/include)
include_directories(/Users/eliyahub/spot/include)

