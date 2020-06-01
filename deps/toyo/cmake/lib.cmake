file(GLOB_RECURSE LIB_SOURCE_FILES "src/lib/*.cpp" "src/lib/*.c")

add_library(${LIB_NAME} STATIC
  ${LIB_SOURCE_FILES}
)

set_target_properties(${LIB_NAME} PROPERTIES CXX_STANDARD 11)

# set_target_properties(${LIB_NAME} PROPERTIES PREFIX "lib")

if(WIN32 AND MSVC)
  target_link_libraries(${LIB_NAME} ntdll Userenv)
  # set_target_properties(${LIB_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  target_compile_options(${LIB_NAME} PRIVATE /utf-8)
  target_compile_definitions(${LIB_NAME} PRIVATE
    _CRT_SECURE_NO_WARNINGS
    UNICODE
    _UNICODE
  )
else()
  target_link_libraries(${LIB_NAME} dl)
endif()

target_include_directories(${LIB_NAME}
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
