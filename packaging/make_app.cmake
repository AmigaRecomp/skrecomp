# The macOS app: Soccer Kid.app with the game binary, SDL3 inside it, and the icon made from your soccerkid.dat.
# Run by the `app` target with BIN, SKICON, DATA, PLIST, VERSION and OUT.
set(APP "${OUT}/Soccer Kid.app")
file(REMOVE_RECURSE "${APP}")
file(MAKE_DIRECTORY "${APP}/Contents/MacOS" "${APP}/Contents/Resources" "${APP}/Contents/Frameworks")
file(COPY_FILE "${BIN}" "${APP}/Contents/MacOS/soccerkid")
configure_file("${PLIST}" "${APP}/Contents/Info.plist" @ONLY)

if(EXISTS "${DATA}")                                    # the icon: the kid from the title picture
  set(ICONSET "${OUT}/SoccerKid.iconset")
  file(REMOVE_RECURSE "${ICONSET}")
  file(MAKE_DIRECTORY "${ICONSET}")
  execute_process(COMMAND "${SKICON}" "${DATA}" "${ICONSET}" RESULT_VARIABLE rc)
  if(rc EQUAL 0)
    execute_process(COMMAND iconutil -c icns "${ICONSET}" -o "${APP}/Contents/Resources/SoccerKid.icns" RESULT_VARIABLE rc)
  endif()
  if(NOT rc EQUAL 0)
    message(WARNING "no icon: skicon or iconutil failed")
  endif()
else()
  message(WARNING "no data/soccerkid.dat: the app has no game inside and no icon (run skextract, rebuild, then make the app)")
endif()

# SDL3 travels inside the app, so it runs without Homebrew
execute_process(COMMAND otool -L "${APP}/Contents/MacOS/soccerkid" OUTPUT_VARIABLE libs)
string(REGEX MATCH "[^\t\n ]*libSDL3[^ \t\n]*\\.dylib" sdl "${libs}")
if(sdl)
  get_filename_component(name "${sdl}" NAME)
  file(REAL_PATH "${sdl}" real)
  file(COPY_FILE "${real}" "${APP}/Contents/Frameworks/${name}")
  execute_process(COMMAND chmod u+w "${APP}/Contents/Frameworks/${name}")
  execute_process(COMMAND install_name_tool -id "@rpath/${name}" "${APP}/Contents/Frameworks/${name}" ERROR_QUIET)   # re-signed below
  execute_process(COMMAND install_name_tool -change "${sdl}" "@executable_path/../Frameworks/${name}" "${APP}/Contents/MacOS/soccerkid" ERROR_QUIET)
endif()
execute_process(COMMAND codesign --force --sign - "${APP}/Contents/Frameworks/${name}" OUTPUT_QUIET ERROR_QUIET)
execute_process(COMMAND codesign --force --sign - "${APP}" RESULT_VARIABLE rc)   # ad hoc: the changed binaries must be signed again
message(STATUS "made ${APP}")
