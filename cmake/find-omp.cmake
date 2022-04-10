find_package(OpenMP QUIET)

if (NOT DEFINED GLIBC_VERSION) # Only need to check once
	execute_process(COMMAND ldd --version
			OUTPUT_VARIABLE GLIBC_VERSION)
	# Regex over ldd output to determine glibc version
	string(REGEX MATCH "[0-9]+[\\.[0-9]+]*" GLIBC_VERSION "${GLIBC_VERSION}")
endif()

if (GLIBC_VERSION VERSION_GREATER_EQUAL "2.34")
	# Since version 2.34 glibc includes several libraries including libpthread.a
	# "For compatibility" these .a libraries are left in place as empty archives
	# nvcc chokes on these empty compatibility files
	# OpenMP_CXX_LIBRARIES contains an explicit mention of /usr/lib/libpthread.a,
	# which creates an error
	message(STATUS "find-omp.cmake: glibc version >= 2.34 detected")
	message(STATUS "Stripping /usr/lib/libpthread.a from OpenMP libraries list...")
	# Remove /usr/share/libpthread.a from OpenMP_CXX_LIBRARIES
	string(REPLACE "/usr/lib/libpthread.a" "" OpenMP_CXX_LIBRARIES "${OpenMP_CXX_LIBRARIES}")
	# Remove trailing characters
	string(REGEX REPLACE "\\s*;\\s*$" "" OpenMP_CXX_LIBRARIES "${OpenMP_CXX_LIBRARIES}")
	string(REGEX REPLACE "^\\s*;\\s*" "" OpenMP_CXX_LIBRARIES "${OpenMP_CXX_LIBRARIES}")
endif()
