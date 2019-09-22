add_library(mockturtle INTERFACE)
target_include_directories(mockturtle INTERFACE ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(mockturtle INTERFACE ez kitty lorina sparsepp percy)
