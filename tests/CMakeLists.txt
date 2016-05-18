
INCLUDE_DIRECTORIES(.)
add_library(atspi
            atspi/atspi.h
            atspi/atspi.c
            )

INCLUDE(CMakeLists.sub)
set_target_properties(atspi PROPERTIES
         LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/atspi)

INCLUDE(FindPkgConfig)
pkg_check_modules(tests REQUIRED
    gobject-2.0
    dlog
    dbus-1
    ecore
    eldbus
    eina
    tts
    check
    vconf
    elementary
)

FOREACH(flag ${tests_CFLAGS})
	SET(EXTRA_CFLAGS_TESTS "${EXTRA_CFLAGS_TESTS} ${flag}")
ENDFOREACH(flag)


SET(CMAKE_C_FLAGS "${SLP_DEBUG_FLAGS} ${SLP_OPT_FLAGS} ${CMAKE_C_FLAGS_DUMP} ${EXTRA_CFLAGS_TESTS} ${SLP_COMPILER_FLAGS}")
SET(CMAKE_CXX_FLAGS "${SLP_DEBUG_FLAGS} ${SLP_OPT_FLAGS} ${CMAKE_CXX_FLAGS_DUMP} ${EXTRA_CFLAGS_TESTS} ${SLP_COMPILER_FLAGS}")

SET(TESTED_SRCS ${CMAKE_SOURCE_DIR}/src/screen_reader.c
                ${CMAKE_SOURCE_DIR}/src/screen_reader_vconf.c
                ${CMAKE_SOURCE_DIR}/src/screen_reader_spi.c
                ${CMAKE_SOURCE_DIR}/src/screen_reader_tts.c
                ${CMAKE_SOURCE_DIR}/src/dbus_direct_reading_adapter.c
                ${CMAKE_SOURCE_DIR}/src/flat_navi.c)
ADD_DEFINITIONS(-DSCREEN_READER_FLAT_NAVI_TEST_DUMMY_IMPLEMENTATION)

ADD_EXECUTABLE(smart_navi_test_suite smart_navi_suite.c ${TESTED_SRCS})
TARGET_LINK_LIBRARIES(smart_navi_test_suite atspi ${tests_LDFLAGS} ${SLP_LD_PATH_FLAGS} ${SLP_LD_FLAGS} ${SLP_LINKER_FLAGS})
