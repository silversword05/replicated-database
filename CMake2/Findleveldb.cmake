# - Find LevelDB
#
#  LevelDB_INCLUDES  - List of LevelDB includes
#  LevelDB_LIBRARIES - List of libraries when using LevelDB.
#  LevelDB_FOUND     - True if LevelDB found.

# Look for the header file.
find_path(leveldb_INCLUDE NAMES leveldb/db.h
                          PATHS $ENV{HOME}/leveldb/include /opt/local/include /usr/local/include /usr/include
                          DOC "Path in which the file leveldb/db.h is located." )

# Look for the library.
find_library(leveldb_LIBRARY NAMES leveldb
                             PATHS /usr/lib $ENV{HOME}/leveldb/build/lib $ENV{HOME}/leveldb/build
                             DOC "Path to leveldb library." )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(leveldb DEFAULT_MSG leveldb_INCLUDE leveldb_LIBRARY)

if(LEVELDB_FOUND)
  message(STATUS "Found LevelDB (include: ${leveldb_INCLUDE}, library: ${leveldb_LIBRARY})")
  set(leveldb_INCLUDES ${leveldb_INCLUDE})
  set(leveldb_LIBRARIES ${leveldb_LIBRARY})
  mark_as_advanced(leveldb_INCLUDE leveldb_LIBRARY)

  if(EXISTS "${leveldb_INCLUDE}/leveldb/db.h")
    file(STRINGS "${leveldb_INCLUDE}/leveldb/db.h" __version_lines
           REGEX "static const int k[^V]+Version[ \t]+=[ \t]+[0-9]+;")

    foreach(__line ${__version_lines})
      if(__line MATCHES "[^k]+kMajorVersion[ \t]+=[ \t]+([0-9]+);")
        set(LEVELDB_VERSION_MAJOR ${CMAKE_MATCH_1})
      elseif(__line MATCHES "[^k]+kMinorVersion[ \t]+=[ \t]+([0-9]+);")
        set(LEVELDB_VERSION_MINOR ${CMAKE_MATCH_1})
      endif()
    endforeach()

    if(LEVELDB_VERSION_MAJOR AND LEVELDB_VERSION_MINOR)
      set(LEVELDB_VERSION "${LEVELDB_VERSION_MAJOR}.${LEVELDB_VERSION_MINOR}")
    endif()

  endif()
endif()