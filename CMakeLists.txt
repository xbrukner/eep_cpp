cmake_minimum_required(VERSION 2.8.12)
project(eep_cpp)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
aux_source_directory(. SRC_LIST)
execute_process(
        COMMAND erl -noshell -eval
        "io:format(\"~s\", [code:lib_dir(erl_interface)])" -s erlang halt
        OUTPUT_VARIABLE EI_BASE
        RESULT_VARIABLE erlang_error)
if( erlang_error )
    message(FATAL_ERROR "Could not execute erlang: ${erlang_error}")
endif()

include_directories("${EI_BASE}/include")
link_directories("${EI_BASE}/lib")

add_executable(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} ei)
