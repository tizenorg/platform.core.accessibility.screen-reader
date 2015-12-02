CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
ENABLE_TESTING()

# FIND ALL SOURCE IN A SOURCE DIRECTORY
#AUX_SOURCE_DIRECTORY(${CMAKE_SOURCE_DIR}/src SRCS)

SET(SRCS ${CMAKE_SOURCE_DIR}/src/app_tracker.c
	 ${CMAKE_SOURCE_DIR}/src/flat_navi.c
	 ${CMAKE_SOURCE_DIR}/src/main.c
	 ${CMAKE_SOURCE_DIR}/src/screen_reader.c
	 ${CMAKE_SOURCE_DIR}/src/screen_reader_haptic.c
	 ${CMAKE_SOURCE_DIR}/src/screen_reader_spi.c
	 ${CMAKE_SOURCE_DIR}/src/screen_reader_switch.c
	 ${CMAKE_SOURCE_DIR}/src/screen_reader_system.c
	 ${CMAKE_SOURCE_DIR}/src/screen_reader_tts.c
	 ${CMAKE_SOURCE_DIR}/src/screen_reader_vconf.c
	 ${CMAKE_SOURCE_DIR}/src/smart_notification.c
	 ${CMAKE_SOURCE_DIR}/src/lua_engine.c
	 ${CMAKE_SOURCE_DIR}/src/keyboard_tracker.c
)


#INCLUDE FILE
INCLUDE(CMakeLists.sub)
SET(RESOURCE_DIR "${CMAKE_SOURCE_DIR}/res")
SET(LOCALEDIR "/usr/apps/org.tizen.screen-reader/res/locale")
ADD_DEFINITIONS(-DSCRIPTDIR="/usr/apps/org.tizen.screen-reader/res/scripts")

IF("${SEC_FEATURE_TAPI_ENABLE}" STREQUAL "1")
    MESSAGE("SEC_FEATURE_TAPI_ENABLE: ${SEC_FEATURE_TAPI_ENABLE}")
    SET(TAPI_REQUIRED_PKG "tapi")
    SET(SRCS ${SRCS}
	${CMAKE_SOURCE_DIR}/src/dbus_gesture_adapter.c
	${CMAKE_SOURCE_DIR}/src/elm_access_adapter.c
	${CMAKE_SOURCE_DIR}/src/screen_reader_gestures.c
	${CMAKE_SOURCE_DIR}/src/navigator.c
	${CMAKE_SOURCE_DIR}/src/window_tracker.c)
ELSE()
    MESSAGE("SEC_FEATURE_TAPI_ENABLE: ${SEC_FEATURE_TAPI_ENABLE}")
    ADD_DEFINITIONS("-DSCREEN_READER_TV")
ENDIF()

IF("${X11_ENABLED}" STREQUAL "1")
    MESSAGE(STATUS "X11 is enabled")
    SET(CONDITIONAL_PACKAGES ${CONDITIONAL_PACKAGES} "ecore-x")
    ADD_DEFINITIONS("-DX11_ENABLED")
ELSE()
    MESSAGE(STATUS "X11 is disabled - using wayland")
    SET(CONDITIONAL_PACKAGES ${CONDITIONAL_PACKAGES} "ecore-wayland")
ENDIF()

INCLUDE(FindPkgConfig)
pkg_check_modules(pkgs REQUIRED
	bundle
	appcore-efl
	eldbus
	elementary
	ecore
	atspi-2
	gobject-2.0
	dlog
	vconf
	tts
	capi-media-tone-player
	capi-system-device
	capi-network-bluetooth
	notification
	capi-network-wifi
	lua
	${TAPI_REQUIRED_PKG}
	${CONDITIONAL_PACKAGES}
)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS_DUMP "${CMAKE_C_FLAGS}")
SET(CMAKE_CXX_FLAGS_DUMP "${CMAKE_CXX_FLAGS}")
SET(CMAKE_C_FLAGS "${SLP_DEBUG_FLAGS} ${SLP_OPT_FLAGS} ${CMAKE_C_FLAGS} ${EXTRA_CFLAGS} ${SLP_COMPILER_FLAGS} -fPIE")
SET(CMAKE_CXX_FLAGS "${SLP_DEBUG_FLAGS} ${SLP_OPT_FLAGS} ${CMAKE_CXX_FLAGS} ${EXTRA_CFLAGS} ${SLP_COMPILER_FLAGS}")

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})

TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${pkgs_LDFLAGS} ${SLP_LD_PATH_FLAGS} ${SLP_LD_FLAGS} ${SLP_LINKER_FLAGS} "-pie")

# Install 
INSTALL(TARGETS ${PROJECT_NAME} DESTINATION bin)
INSTALL(DIRECTORY ${RESOURCE_DIR}/icons DESTINATION res) 
INSTALL(DIRECTORY ${RESOURCE_DIR}/scripts DESTINATION res)
# Install Manifest File
INSTALL(FILES org.tizen.screen-reader.xml DESTINATION /usr/share/packages)

ADD_SUBDIRECTORY(${CMAKE_SOURCE_DIR}/res/po)
ADD_SUBDIRECTORY(${CMAKE_SOURCE_DIR}/tests)
ADD_TEST(NAME smart_navi_tests COMMAND ${CMAKE_SOURCE_DIR}/tests/smart_navi_test_suite)
# END OF A FILE
