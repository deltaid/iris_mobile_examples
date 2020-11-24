cmake_minimum_required(VERSION 3.10)
project(ire3_extra_images)


add_executable(extra_images_example ire3_extra_images.cpp)
target_link_libraries(extra_images_example PRIVATE iris_engine_v3 utils)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    if (TARGET libjasper)
        add_custom_command(TARGET extra_images_example POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:libjasper>/libjasper.dll $<TARGET_FILE_DIR:extra_images_example>
        )
    endif()

    add_custom_command(TARGET extra_images_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:iris_engine_v3>/iris_engine_v3.dll $<TARGET_FILE_DIR:extra_images_example>
    )
endif()                                                