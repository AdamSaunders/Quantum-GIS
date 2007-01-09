
# CMake module to search for PostgreSQL library
#
# pg_config is searched for in POSTGRES_CONFIG dir,
# default /usr/bin
#
# If it's found it sets POSTGRES_FOUND to TRUE
# and following variables are set:
#    POSTGRES_INCLUDE_DIR
#    POSTGRES_LIBRARY

IF(WIN32)
  IF (NOT POSTGRES_INCLUDE_DIR)
    FIND_PATH(POSTGRES_INCLUDE_DIR libpq-fe.h /usr/local/include /usr/include)
  ENDIF (NOT POSTGRES_INCLUDE_DIR)

  IF (NOT POSTGRES_LIBRARY)
    FIND_LIBRARY(POSTGRES_LIBRARY NAMES pq PATH /usr/local/lib /usr/lib)
  ENDIF (NOT POSTGRES_LIBRARY)

ELSE(WIN32)
  IF(UNIX) 

    SET(POSTGRES_CONFIG_PREFER_PATH "$ENV{POSTGRES_HOME}/bin" CACHE STRING "preferred path to PG (pg_config)")
    FIND_PROGRAM(POSTGRES_CONFIG pg_config
      ${POSTGRES_CONFIG_PREFER_PATH}
      /usr/bin/
      )
    # MESSAGE("DBG POSTGRES_CONFIG ${POSTGRES_CONFIG}")
    
    IF (POSTGRES_CONFIG) 
      # set INCLUDE_DIR
      EXEC_PROGRAM(${POSTGRES_CONFIG}
        ARGS --includedir
        OUTPUT_VARIABLE PG_TMP)
      SET(POSTGRES_INCLUDE_DIR ${PG_TMP} CACHE STRING INTERNAL)

      # set LIBRARY_DIR
      EXEC_PROGRAM(${POSTGRES_CONFIG}
        ARGS --libdir
        OUTPUT_VARIABLE PG_TMP)
      SET(POSTGRES_LIBRARY ${PG_TMP}/libpq.so CACHE STRING INTERNAL)
    ENDIF(POSTGRES_CONFIG)

  ENDIF(UNIX)
ENDIF(WIN32)

IF (POSTGRES_INCLUDE_DIR AND POSTGRES_LIBRARY)
   SET(POSTGRES_FOUND TRUE)
ENDIF (POSTGRES_INCLUDE_DIR AND POSTGRES_LIBRARY)


IF (POSTGRES_FOUND)

   IF (NOT POSTGRES_FIND_QUIETLY)
      MESSAGE(STATUS "Found PostgreSQL: ${POSTGRES_LIBRARY}")
   ENDIF (NOT POSTGRES_FIND_QUIETLY)

ELSE (POSTGRES_FOUND)

   #SET (POSTGRES_INCLUDE_DIR "")
   #SET (POSTGRES_LIBRARY "")

   IF (POSTGRES_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find PostgreSQL")
   ELSE (POSTGRES_FIND_REQUIRED)
      MESSAGE(STATUS "Could not find PostgreSQL")
   ENDIF (POSTGRES_FIND_REQUIRED)

ENDIF (POSTGRES_FOUND)
