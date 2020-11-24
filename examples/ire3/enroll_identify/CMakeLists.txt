cmake_minimum_required(VERSION 3.10)
project(ire3_enroll_identify)


add_executable(ire3_enroll_identify_example ire3_enroll_identify.cpp)
target_link_libraries(ire3_enroll_identify_example PRIVATE iris_engine_v3 utils)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    if (TARGET libjasper)
        add_custom_command(TARGET ire3_enroll_identify_example POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:libjasper>/libjasper.dll $<TARGET_FILE_DIR:ire3_enroll_identify_example>
        )
    endif()

    add_custom_command(TARGET ire3_enroll_identify_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:iris_engine_v3>/iris_engine_v3.dll $<TARGET_FILE_DIR:ire3_enroll_identify_example>
    )
endif()                                                