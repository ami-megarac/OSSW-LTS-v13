find_path(HIREDIS_INCLUDE_DIRS hiredis/hiredis.h HINTS "/usr/include")
find_library(HIREDIS_LIB_HIREDIS NAMES hiredis HINTS "/usr/lib")

set(HIREDIS_LIBRARIES ${HIREDIS_LIB_HIREDIS})

