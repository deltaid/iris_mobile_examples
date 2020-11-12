cmake_minimum_required(VERSION 3.10)
project(enroll_identify)

add_executable(enroll_identify example.cpp)
target_link_libraries(enroll_identify PRIVATE iris_mobile_v2 iris_image_record utils)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_custom_command(TARGET enroll_identify POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:iris_mobile_v2>/iris_mobile_v2.dll $<TARGET_FILE_DIR:enroll_identify>
    )
endif()                                                