####################################################################################################
#####################              NOA unit & functional testing             #######################
####################################################################################################

message(STATUS "Building tests required")


# Functional tests
add_subdirectory(cfd)
add_subdirectory(pms)

# Unit tests
add_subdirectory(unit)
