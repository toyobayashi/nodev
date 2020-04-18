file(GLOB_RECURSE EXE_SOURCE_FILES "src/*.cpp" "src/*.c")

if(WIN32 AND MSVC)
  set(NODEV_VERSIONINFO_RC "${CMAKE_CURRENT_BINARY_DIR}/nodev.rc")
  configure_file("${CMAKE_CURRENT_SOURCE_DIR}/src/nodev.rc.in" "${NODEV_VERSIONINFO_RC}")
else()
  set(NODEV_VERSIONINFO_RC "")
endif()

add_executable(${EXE_NAME} ${EXE_SOURCE_FILES}
  "deps/zlib/contrib/minizip/unzip.c"
  "deps/zlib/contrib/minizip/ioapi.c"
  ${NODEV_VERSIONINFO_RC}
)

set_target_properties(${EXE_NAME} PROPERTIES CXX_STANDARD 11)

target_compile_definitions(${EXE_NAME} PRIVATE CURL_STATICLIB NODEV_VERSION="${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")

if(WIN32 AND MSVC)
  target_link_libraries(${EXE_NAME}
    # "${CMAKE_CURRENT_SOURCE_DIR}/deps/curl/builds/libcurl-vc16-x86-release-static-ipv6-sspi-winssl/lib/libcurl_a.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/libcurl_a.lib"
    # "${CMAKE_CURRENT_SOURCE_DIR}/deps/zlib/contrib/vstudio/vc16/x86/ZlibDllRelease/zlibwapi.lib"
    # "${CMAKE_CURRENT_SOURCE_DIR}/deps/zlib/zdll.lib"
    "${CMAKE_CURRENT_SOURCE_DIR}/lib/zlib.lib"
    ws2_32
    wldap32
    crypt32
    Normaliz
  )
  target_include_directories(${EXE_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/curl/include"
    "deps/zlib"
  )
  # set_target_properties(${EXE_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  target_compile_options(${EXE_NAME} PRIVATE /utf-8)
  target_compile_definitions(${EXE_NAME} PRIVATE
    _CRT_SECURE_NO_WARNINGS
    UNICODE
    _UNICODE
  )

  # target_compile_definitions(${EXE_NAME} PRIVATE ZLIB_WINAPI)

  if(${CCPM_BUILD_TYPE} MATCHES Debug)
    target_link_options(${EXE_NAME} PRIVATE /NODEFAULTLIB:"libcmt.lib" /NODEFAULTLIB:"msvcrt.lib")
  endif()
else()
  if(NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath=$ORIGIN")
    add_subdirectory("deps/curl")

    target_link_libraries(${EXE_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/lib/libz.a" libcurl)
    target_include_directories(${EXE_NAME} PRIVATE "deps/zlib")
  else()
    # target_compile_options(${EXE_NAME} PRIVATE -pthread)
    # set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework CoreFoundation -framework Security")
    target_link_libraries(${EXE_NAME} "${CMAKE_CURRENT_SOURCE_DIR}/lib/libz.a" "${CMAKE_CURRENT_SOURCE_DIR}/lib/libcurl.a"
      idn2
      # ldap
      "-framework CoreFoundation"
      "-framework Security"
    )
    target_include_directories(${EXE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/deps/curl/include" "deps/zlib")
  endif()
endif()

add_subdirectory("deps/toyo")
target_link_libraries(${EXE_NAME} toyo)
