# Write the test script
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/runtest.sh.in
               ${CMAKE_CURRENT_BINARY_DIR}/runtest.sh)

# Add a test running the testscript
add_test(NAME dependent-project COMMAND ${CMAKE_CURRENT_BINARY_DIR}/runtest.sh)
