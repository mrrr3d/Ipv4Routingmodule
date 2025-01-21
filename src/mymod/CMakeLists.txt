check_include_file_cxx(stdint.h HAVE_STDINT_H)
if(HAVE_STDINT_H)
    add_definitions(-DHAVE_STDINT_H)
endif()

set(examples_as_tests_sources)
if(${ENABLE_EXAMPLES})
    set(examples_as_tests_sources
        #test/mymod-examples-test-suite.cc
        )
endif()

build_lib(
    LIBNAME mymod
    SOURCE_FILES model/mymod.cc
                 helper/mymod-helper.cc
                 model/myipv4routing.cc
    HEADER_FILES model/mymod.h
                 helper/mymod-helper.h
                 model/myipv4routing.h
    LIBRARIES_TO_LINK 
                 ${libcore}
                 ${libinternet}
                 ${libnetwork}
    TEST_SOURCES test/mymod-test-suite.cc
                 ${examples_as_tests_sources}
)
