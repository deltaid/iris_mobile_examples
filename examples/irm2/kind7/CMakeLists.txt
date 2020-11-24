cmake_minimum_required(VERSION 3.10)
project(kind7)

add_executable(kind7_example kind7_example.cpp)
target_link_libraries(kind7_example PRIVATE iris_mobile_v2 iris_image_record utils)

if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    add_custom_command(TARGET kind7_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:iris_mobile_v2>/iris_mobile_v2.dll $<TARGET_FILE_DIR:kind7_example>
    )
    add_custom_command(TARGET kind7_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:iris_image_record>/iris_image_record.dll $<TARGET_FILE_DIR:kind7_example>
    )
    add_custom_command(TARGET kind7_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:libjasper>/libjasper.dll $<TARGET_FILE_DIR:kind7_example>
    )
    add_custom_command(TARGET kind7_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE_DIR:iris_engine_v3>/iris_engine_v3.dll $<TARGET_FILE_DIR:kind7_example>
    )
endif()                                                