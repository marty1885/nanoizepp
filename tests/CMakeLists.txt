find_package(Catch2 3.0 REQUIRED)
add_executable(nanoizepp-test nanoizepp-test.cpp)
target_link_libraries(nanoizepp-test PRIVATE nanoizepp Catch2::Catch2WithMain)