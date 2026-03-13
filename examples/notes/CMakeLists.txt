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
endif()
