file(GLOB TEST_CODE_SRC CONFIGURE_DEPENDS "*.cc")

file(GLOB CODE_FILES "${PROJECT_SOURCE_DIR}/*.cc")
list(REMOVE_ITEM CODE_FILES ${PROJECT_SOURCE_DIR}/raft-main.cc)
list(REMOVE_ITEM CODE_FILES ${PROJECT_SOURCE_DIR}/client-server.cc)

foreach(SOURCE_PATH ${TEST_CODE_SRC})
    get_filename_component(EXECUTABLE_NAME ${SOURCE_PATH} NAME_WE)
    add_executable(${EXECUTABLE_NAME} ${SOURCE_PATH} ${CODE_FILES})
    target_compile_options(${EXECUTABLE_NAME} PRIVATE ${COMPILE_OPTIONS})
    target_compile_features(${EXECUTABLE_NAME} PRIVATE ${COMPILE_FEATURES})
    target_include_directories(${EXECUTABLE_NAME} PRIVATE ${PROJECT_SOURCE_DIR}/includes)
    target_link_libraries(${EXECUTABLE_NAME} PRIVATE myproto)
endforeach(SOURCE_PATH ${SOURCE_FILES})