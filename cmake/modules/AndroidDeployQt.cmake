find_package(Qt5Core REQUIRED)

function(androiddeployqt QTANDROID_EXPORTED_TARGET ADDITIONAL_FIND_ROOT_PATH)
    set(EXPORT_DIR "${CMAKE_BINARY_DIR}/${QTANDROID_EXPORTED_TARGET}_build_apk/")
    set(EXECUTABLE_DESTINATION_PATH "${EXPORT_DIR}/libs/${CMAKE_ANDROID_ARCH_ABI}/lib${QTANDROID_EXPORTED_TARGET}.so")

    set(EXTRA_PREFIX_DIRS "")
    foreach(prefix ${ADDITIONAL_FIND_ROOT_PATH})
        if (EXTRA_PREFIX_DIRS)
            set(EXTRA_PREFIX_DIRS "${EXTRA_PREFIX_DIRS}, \"${prefix}\"")
        else()
            set(EXTRA_PREFIX_DIRS "\"${prefix}\"")
        endif()
    endforeach()
    string(TOLOWER "${CMAKE_HOST_SYSTEM_NAME}" _LOWER_CMAKE_HOST_SYSTEM_NAME)
    configure_file("${_CMAKE_ANDROID_DIR}/deployment-file.json.in" "${CMAKE_BINARY_DIR}/${QTANDROID_EXPORTED_TARGET}-deployment.json.in1")
    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/${QTANDROID_EXPORTED_TARGET}-deployment.json.in2"
                  INPUT  "${CMAKE_BINARY_DIR}/${QTANDROID_EXPORTED_TARGET}-deployment.json.in1")

    if (NOT TARGET create-apk)
        add_custom_target(create-apk)
    endif()

    set(CREATEAPK_TARGET_NAME "create-apk-${QTANDROID_EXPORTED_TARGET}")
    add_custom_target(${CREATEAPK_TARGET_NAME}
        COMMAND cmake -E echo "Generating $<TARGET_NAME:${QTANDROID_EXPORTED_TARGET}> with $<TARGET_FILE_DIR:Qt5::qmake>/androiddeployqt"
        COMMAND cmake -E remove_directory "${EXPORT_DIR}"
        COMMAND cmake -E copy_directory "$<TARGET_PROPERTY:create-apk-${QTANDROID_EXPORTED_TARGET},ANDROID_APK_DIR>" "${EXPORT_DIR}"
        COMMAND cmake -E copy "$<TARGET_FILE:${QTANDROID_EXPORTED_TARGET}>" "${EXECUTABLE_DESTINATION_PATH}"
        COMMAND LANG=C cmake "-DTARGET=$<TARGET_FILE:${QTANDROID_EXPORTED_TARGET}>" -P ${_CMAKE_ANDROID_DIR}/hasMainSymbol.cmake
        COMMAND LANG=C cmake -DINPUT_FILE="${QTANDROID_EXPORTED_TARGET}-deployment.json.in2" -DOUTPUT_FILE="${QTANDROID_EXPORTED_TARGET}-deployment.json" "-DTARGET=$<TARGET_FILE:${QTANDROID_EXPORTED_TARGET}>" "-DOUTPUT_DIR=$<TARGET_FILE_DIR:${QTANDROID_EXPORTED_TARGET}>" "-DEXPORT_DIR=${CMAKE_INSTALL_PREFIX}" "-DANDROID_ADDITIONAL_FIND_ROOT_PATH=\"${ANDROID_ADDITIONAL_FIND_ROOT_PATH}\"" "-DANDROID_EXTRA_LIBS=\"${ANDROID_EXTRA_LIBS}\"" "-DANDROID_EXTRA_PLUGINS=\"${ANDROID_EXTRA_PLUGINS}\"" -P ${_CMAKE_ANDROID_DIR}/specifydependencies.cmake
        COMMAND $<TARGET_FILE_DIR:Qt5::qmake>/androiddeployqt --gradle --input "${QTANDROID_EXPORTED_TARGET}-deployment.json" --output "${EXPORT_DIR}" --deployment bundled "\\$(ARGS)"
    )

    add_custom_target(install-apk-${QTANDROID_EXPORTED_TARGET}
        COMMAND adb install -r "${EXPORT_DIR}/build/outputs/apk/${QTANDROID_EXPORTED_TARGET}_build_apk-debug.apk"
    )
    add_dependencies(create-apk ${CREATEAPK_TARGET_NAME})
endfunction()
