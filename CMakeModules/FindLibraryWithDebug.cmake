#
#  FIND_LIBRARY_WITH_DEBUG
#  -> enhanced FIND_LIBRARY to allow the search for an
#     optional debug library with a WIN32_DEBUG_POSTFIX similar
#     to CMAKE_DEBUG_POSTFIX when creating a shared lib
#     it has to be the second and third argument

# Copyright (c) 2007, Christian Ehrlicher, <ch.ehrlicher@gmx.de>
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

MACRO(FIND_LIBRARY_WITH_DEBUG var_name win32_dbg_postfix_name dgb_postfix libname)

  IF((NOT "${win32_dbg_postfix_name}" STREQUAL "WIN32_DEBUG_POSTFIX") AND (NOT VCPKG_TOOLCHAIN))

     # no WIN32_DEBUG_POSTFIX -> simply pass all arguments to FIND_LIBRARY
     FIND_LIBRARY(${var_name}
                  ${win32_dbg_postfix_name}
                  ${dgb_postfix}
                  ${libname}
                  ${ARGN}
     )

  ELSE()

    IF(NOT WIN32)
      # on non-win32 we don't need to take care about WIN32_DEBUG_POSTFIX

      FIND_LIBRARY(${var_name} ${libname} ${ARGN})

    ELSE(NOT WIN32)

      # 1. get all possible libnames
      IF(VCPKG_TOOLCHAIN AND (NOT "${win32_dbg_postfix_name}" STREQUAL "WIN32_DEBUG_POSTFIX"))
        SET(args ${win32_dbg_postfix_name})
        LIST(APPEND args ${dgb_postfix})
        LIST(APPEND args ${ARGN})
      ELSE()
        SET(args ${ARGN})
      ENDIF()
      SET(newargs "")
      SET(libnames_release "")
      SET(libnames_debug "")

      LIST(LENGTH args listCount)

      IF("${libname}" STREQUAL "NAMES")
        SET(append_rest 0)
        LIST(APPEND args " ")

        FOREACH(i RANGE ${listCount})
          LIST(GET args ${i} val)

          IF(append_rest)
            LIST(APPEND newargs ${val})
          ELSE(append_rest)
            IF("${val}" STREQUAL "PATHS")
              LIST(APPEND newargs ${val})
              SET(append_rest 1)
            ELSE("${val}" STREQUAL "PATHS")
              LIST(APPEND libnames_release "${val}")
              LIST(APPEND libnames_debug   "${val}${dgb_postfix}")
              IF(VCPKG_TOOLCHAIN)
                # in VCPKG the debug library does often not contain a postfix, 
                # so search also for this variant
                LIST(APPEND libnames_debug "${val}")
              ENDIF()
            ENDIF("${val}" STREQUAL "PATHS")
          ENDIF(append_rest)

        ENDFOREACH(i)

      ELSE("${libname}" STREQUAL "NAMES")

        # just one name
        LIST(APPEND libnames_release "${libname}")
        LIST(APPEND libnames_debug   "${libname}${dgb_postfix}")
        IF(VCPKG_TOOLCHAIN)
          # in VCPKG the debug library does often not contain a postfix, 
          # so search also for this variant
          LIST(APPEND libnames_debug "${val}")
        ENDIF()

        SET(newargs ${args})

      ENDIF("${libname}" STREQUAL "NAMES")

      # search the release lib
      FIND_LIBRARY(${var_name}_RELEASE
                   NAMES ${libnames_release}
                   ${newargs}
      )

      # search the debug lib
      IF(VCPKG_TOOLCHAIN)
        FIND_LIBRARY(${var_name}_DEBUG
                     NAMES ${libnames_debug}
                     PATHS ${_VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/debug/lib
                     ${newargs}
                     NO_DEFAULT_PATH
          )
      ELSE()
        FIND_LIBRARY(${var_name}_DEBUG
                     NAMES ${libnames_debug}
                     ${newargs}
        )
      ENDIF()

      IF(${var_name}_RELEASE AND ${var_name}_DEBUG)

        # both libs found
        SET(${var_name} optimized ${${var_name}_RELEASE}
                        debug     ${${var_name}_DEBUG})

      ELSE(${var_name}_RELEASE AND ${var_name}_DEBUG)

        IF(${var_name}_RELEASE)

          # only release found
          SET(${var_name} ${${var_name}_RELEASE})

        ELSE(${var_name}_RELEASE)

          # only debug (or nothing) found
          SET(${var_name} ${${var_name}_DEBUG})

        ENDIF(${var_name}_RELEASE)
       
      ENDIF(${var_name}_RELEASE AND ${var_name}_DEBUG)

      MARK_AS_ADVANCED(${var_name}_RELEASE)
      MARK_AS_ADVANCED(${var_name}_DEBUG)

    ENDIF(NOT WIN32)

  ENDIF()

ENDMACRO(FIND_LIBRARY_WITH_DEBUG)
