
set(TEST_LDP_IN_SRCS
        ldp1000_tests.cpp
		ldv1000_supermode_tests.cpp
		ldv1000_tests.cpp
		pr7820_tests.cpp
		pr8210_tests.cpp
		vip9500sg_tests.cpp
		vp931_tests.cpp
		vp932_tests.cpp
        stdafx.h
        mocks.h
		ld700_tests.cpp
		ld700_test_interface.h
)

add_executable(test_ldp_in ${TEST_LDP_IN_SRCS})

# should help improve compile time for test project
target_precompile_headers(test_ldp_in PRIVATE stdafx.h)

# this will automatically give the indicated targets access to the headers/libs of the indicated dependencies
target_link_libraries(test_ldp_in LINK_PUBLIC ldp_in gmock gtest_main)
