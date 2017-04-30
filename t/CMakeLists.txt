include_directories("${RTS_SOURCE_DIR}/src" "${RTS_SOURCE_DIR}/opt/Catch/include")
add_definitions(-DBOOST_TEST_DYN_LINK)

function(test exe)
  add_executable(${exe} ${exe}.cpp)
  target_link_libraries(${exe} rts ${Boost_LIBRARIES} ${Event_LIBRARIES})
  #if(USE_COVERAGE) # and gcc
    #target_link_libraries(${exe} gcov)
  #endif()
  add_test(NAME ${exe} COMMAND ${exe})
endfunction(test)

file(GLOB tests *.cpp)
list(SORT tests)
foreach(file ${tests})
	get_filename_component(name_without_extension ${file} NAME_WE)
	test(${name_without_extension})
endforeach()