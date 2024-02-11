add_library( doctest INTERFACE )
target_include_directories( doctest INTERFACE . )


set( WERKZEUG_TEST_SANITIZE True CACHE BOOL "Whether to add the sanitizer")
if ( WERKZEUG_TEST_SANITIZE )
	message(STATUS "Adding Sanitizer")
	set(WERKZEUG_TEST_COMPILE_FLAGS ${WERKZEUG_TEST_COMPILE_FLAGS} -fsanitize=address,undefined )
	set(WERKZEUG_TEST_LINK_FLAGS ${WERKZEUG_TEST_LINK_FLAGS} -fsanitize=address,undefined )
endif ()

set( WERKZEUG_TEST_listdcpp_debug True CACHE BOOL "Whether to debugging to libstdc++")
if ( WERKZEUG_TEST_listdcpp_debug )
	message(STATUS "Adding libstdc++ debug checks")
	set(WERKZEUG_TEST_COMPILE_FLAGS ${WERKZEUG_TEST_COMPILE_FLAGS} -D_GLIBCXX_DEBUG )
endif ()



function( werkzeug_add_test file_without_cpp )
    message( STATUS "Adding test file ${file_without_cpp}" )
    add_executable( ${file_without_cpp}
        "${file_without_cpp}.cpp"
    )
    target_link_libraries( ${file_without_cpp} PRIVATE werkzeug doctest )
	set_target_properties( ${file_without_cpp} PROPERTIES CMAKE_EXPORT_COMPILE_COMMANDS On )
    add_test( NAME ${file_without_cpp} COMMAND ${file_without_cpp} )
	if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_compile_options( ${file_without_cpp} PRIVATE -fconcepts-diagnostics-depth=3 )
	endif()
	target_compile_options( ${file_without_cpp} PRIVATE ${WERKZEUG_TEST_COMPILE_FLAGS} )
	target_link_options( ${file_without_cpp} PRIVATE ${WERKZEUG_TEST_LINK_FLAGS} )
endfunction()

werkzeug_add_test( test_sorting )
werkzeug_add_test( test_stack_dynamic_array )
werkzeug_add_test( test_dynamic_array )
werkzeug_add_test( test_list )
werkzeug_add_test( test_unique_ptr )
werkzeug_add_test( test_expected )
werkzeug_add_test( test_c_api_interfacing )
werkzeug_add_test( test_inheritance_variant )
werkzeug_add_test( test_state_machine )
werkzeug_add_test( test_manual_lifetime )
werkzeug_add_test( test_parameter_pack )
werkzeug_add_test( test_type_erased_object )
werkzeug_add_test( test_polymorphic_list )