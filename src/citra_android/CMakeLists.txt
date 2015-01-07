set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(SRCS
            emu_window/emu_window_android.cpp
            citra.cpp
            config.cpp
            citra.rc
            )
set(HEADERS
            emu_window/emu_window_android.h
            config.h
            default_ini.h
            resource.h
            )

create_directory_groups(${SRCS} ${HEADERS})

add_library(citra SHARED ${SRCS} ${HEADERS})
target_link_libraries(citra core common video_core)
target_link_libraries(citra GLESv2 EGL inih)

if (UNIX)
    target_link_libraries(citra -pthread)
endif()

add_custom_command(TARGET citra POST_BUILD
COMMAND mkdir ARGS -p ${CMAKE_SOURCE_DIR}/src/android/libs/${ANDROID_NDK_ABI_NAME}
)
add_custom_command(TARGET citra POST_BUILD
COMMAND cp ARGS ${CMAKE_CURRENT_BINARY_DIR}/libcitra.so ${CMAKE_SOURCE_DIR}/src/android/libs/${ANDROID_NDK_ABI_NAME}/
)

#install(TARGETS citra RUNTIME DESTINATION ${bindir})
