file(GLOB_RECURSE TEST_SOURCE_FILES "test/test.cpp" "test/regex.cpp" "test/main.cpp")

add_executable(${TEST_EXE_NAME}
  ${TEST_SOURCE_FILES}
)

set_target_properties(${TEST_EXE_NAME} PROPERTIES CXX_STANDARD 11)

target_link_libraries(${TEST_EXE_NAME} ${LIB_NAME})

if(WIN32 AND MSVC)
  set_directory_properties(PROPERTIES VS_STARTUP_PROJECT ${TEST_EXE_NAME})
  # set_target_properties(${TEST_EXE_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  target_compile_options(${TEST_EXE_NAME} PRIVATE /utf-8)
  target_compile_definitions(${TEST_EXE_NAME} PRIVATE
    _CRT_SECURE_NO_WARNINGS
    UNICODE
    _UNICODE
  )
endif()

# if(WIN32 AND MSVC)
#   set(NODE_ADDON_NAME addon)
#   add_library(${NODE_ADDON_NAME} SHARED
#     "test/addon.cpp"
#     "test/test.cpp"
#   )

#   set_target_properties(${NODE_ADDON_NAME}
#     PROPERTIES SUFFIX ".node" CXX_STANDARD 11
#   )

#   target_include_directories(${NODE_ADDON_NAME} PRIVATE "C:\\Users\\toyo\\AppData\\Local\\node-gyp\\Cache\\12.16.3\\include")
#   target_link_libraries(${NODE_ADDON_NAME} ${LIB_NAME} "C:\\Users\\toyo\\AppData\\Local\\node-gyp\\Cache\\12.16.3\\ia32\\node.lib")

#   target_compile_options(${NODE_ADDON_NAME} PRIVATE /utf-8)
#   target_compile_definitions(${NODE_ADDON_NAME} PRIVATE
#     _CRT_SECURE_NO_WARNINGS
#     UNICODE
#     _UNICODE
#   )
# endif()
