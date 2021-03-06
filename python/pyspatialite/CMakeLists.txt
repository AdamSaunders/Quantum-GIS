INCLUDE_DIRECTORIES(
  ../../src/core/spatialite/headers
  ../../src/core/spatialite/headers/spatialite

  ${PYTHON_INCLUDE_PATH}
  ${GEOS_INCLUDE_DIR}
  ${PROJ_INCLUDE_DIR}
)

SET(PYSPATIALITE_SRC
	src/cache.c
	src/connection.c
	src/cursor.c
	src/microprotocols.c
	src/module.c
	src/prepare_protocol.c
	src/row.c
	src/statement.c
	src/util.c
)

ADD_DEFINITIONS(-DMODULE_NAME=\\\"spatialite.dbapi2\\\")

IF (CYGWIN OR APPLE)
  ADD_LIBRARY(pyspatialite MODULE ${PYSPATIALITE_SRC})
ELSE (CYGWIN OR APPLE)
  ADD_LIBRARY(pyspatialite SHARED ${PYSPATIALITE_SRC})
ENDIF (CYGWIN OR APPLE)

IF (NOT APPLE)
  TARGET_LINK_LIBRARIES(pyspatialite ${PYTHON_LIBRARY})
ENDIF (NOT APPLE)

TARGET_LINK_LIBRARIES(pyspatialite ${EXTRA_LINK_LIBRARIES})

IF (APPLE)
  SET_TARGET_PROPERTIES(pyspatialite PROPERTIES LINK_FLAGS "-undefined dynamic_lookup")
ENDIF (APPLE)

SET_TARGET_PROPERTIES(pyspatialite PROPERTIES PREFIX "" OUTPUT_NAME _spatialite)
    
IF (WIN32)
  SET_TARGET_PROPERTIES(pyspatialite PROPERTIES SUFFIX ".pyd")
ENDIF (WIN32)

INSTALL(TARGETS pyspatialite DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/pyspatialite")
INSTALL(FILES
	lib/__init__.py
	lib/dbapi2.py
	lib/dump.py
	DESTINATION "${PYTHON_SITE_PACKAGES_DIR}/pyspatialite")
