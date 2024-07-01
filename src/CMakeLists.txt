
# for convenience
set(header_path "${CMAKE_SOURCE_DIR}/include/ldp-in")

# headers to be installed
set(LDP_IN_PUBLIC_INCLUDE
		${header_path}/vp932-interpreter.h
		${header_path}/ldp1000-interpreter.h
		${header_path}/ldv1000-interpreter.h
		${header_path}/pr7820-interpreter.h
		${header_path}/pr8210-interpreter.h
		${header_path}/vip9500sg-interpreter.h
		${header_path}/vp931-interpreter.h
		)

# source files to be built
set(LDP_IN_SRCS
		ldp1000-interpreter.c
		ldv1000-interpreter.c
		pr7820-interpreter.c
		pr8210-interpreter.c
		vip9500sg-interpreter.c
		vp931-interpreter.c
		vp932-interpreter.c
        )

add_library(ldp_in ${LDP_IN_PUBLIC_INCLUDE} ${LDP_IN_SRCS} )

# So that anything that links to our lib gets the headers for all dependencies
target_include_directories(ldp_in PUBLIC

		# this will be populated when installing but blank when building
		# for client in install mode
		$<INSTALL_INTERFACE:${include_dest}>

		# this will be populated when building but blank when installing
		$<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>)

install(FILES ${LDP_IN_PUBLIC_INCLUDE} DESTINATION "${include_dest}/ldp-in")

# goes into lib_dest so that Debug/Release is taken into account
install(TARGETS ldp_in EXPORT ldp_in DESTINATION "${lib_dest}")