add_executable(app_notes main.c)
target_link_libraries(app_notes PRIVATE c-rest-framework)

if(BUILD_TESTING)
    # The note_model object is defined in main.c, but we don't want to redefine it or have multiple mains.
    # To fix this, let's just add both main.c and test_notes.c, but wait, main.c has a main() function!
    # I should split main.c or define note_model in test_notes.c for the test executable.
    
    # Wait, I can just build test_notes.c and define note_model there.
    add_executable(test_notes test_notes.c)
    target_link_libraries(test_notes PRIVATE c-rest-framework)
    add_test(NAME test_notes COMMAND test_notes)

    if(WIN32 AND BUILD_SHARED_LIBS)
        if(TARGET cfs)
            add_custom_command(TARGET test_notes POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:cfs>" "$<TARGET_FILE_DIR:test_notes>"
            )
        endif()
        if(TARGET c89stringutils)
            add_custom_command(TARGET test_notes POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c89stringutils>" "$<TARGET_FILE_DIR:test_notes>"
            )
        endif()
        if(TARGET c-abstract-http)
            add_custom_command(TARGET test_notes POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c-abstract-http>" "$<TARGET_FILE_DIR:test_notes>"
            )
        endif()
        if(TARGET c_orm)
            add_custom_command(TARGET test_notes POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c_orm>" "$<TARGET_FILE_DIR:test_notes>"
            )
        endif()
        if(TARGET c-rest-framework)
            add_custom_command(TARGET test_notes POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:c-rest-framework>" "$<TARGET_FILE_DIR:test_notes>"
            )
        endif()
    endif()
endif()

