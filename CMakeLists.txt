
cmake_minimum_required( VERSION 3.21 )

project( werkzeug VERSION 1.0 )

add_library( werkzeug INTERFACE )
target_compile_features(werkzeug INTERFACE cxx_std_20 )

target_include_directories( werkzeug INTERFACE include )


set( WERKEZEUG_CXX_WARN_FLAGS "" CACHE STRING "compiler (warning) flags")
if ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang" )
	message( STATUS "Adding g++ & clang++ shared flags" )
	set( WERKEZEUG_CXX_WARN_FLAGS -Wall -Wextra -Wshadow -Wnon-virtual-dtor -Wold-style-cast -Wcast-align -Wzero-as-null-pointer-constant)
	set( WERKEZEUG_CXX_WARN_FLAGS ${WERKEZEUG_CXX_WARN_FLAGS} -Wunused -Woverloaded-virtual -Wpedantic -Wconversion -Wsign-conversion -Wfloat-conversion -Wpessimizing-move )
	set( WERKEZEUG_CXX_WARN_FLAGS ${WERKEZEUG_CXX_WARN_FLAGS} -Wformat=2)
endif ()


if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	message(STATUS "Adding g++ specific flags")
	set(WERKEZEUG_CXX_WARN_FLAGS ${WERKEZEUG_CXX_WARN_FLAGS} -Werror=vla -Wmisleading-indentation -Wduplicated-cond -Wduplicated-branches)
	set(WERKEZEUG_CXX_WARN_FLAGS ${WERKEZEUG_CXX_WARN_FLAGS} -Wlogical-op -Wnull-dereference -Wno-aggressive-loop-optimizations )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	message(STATUS "Adding clang specific flags")
	set(WERKEZEUG_CXX_WARN_FLAGS ${WERKEZEUG_CXX_WARN_FLAGS} -Wpointer-bool-conversion -Wempty-body -Wno-undefined-var-template )
elseif( CMAKE_CXX_COMPILER_ID STREQUAL "Intel" )
	message(STATUS "Adding ICC warning flags")
	set(WERKEZEUG_CXX_WARN_FLAGS ${WERKEZEUG_CXX_WARN_FLAGS} -Weffc++ -W3)
endif()

target_compile_options( werkzeug INTERFACE ${WERKEZEUG_CXX_WARN_FLAGS} )


if ( PROJECT_IS_TOP_LEVEL )
	set( BUILD_TESTING False CACHE BOOL "Whether to build tests" )
	if ( BUILD_TESTING )
		add_subdirectory(tests)
		enable_testing()
	endif ()
endif ()