add_library( doctest INTERFACE )
target_include_directories( doctest INTERFACE . )


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