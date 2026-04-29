cmake_minimum_required(VERSION 3.22)

project(Flashcards VERSION 1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOGEN_ORIGIN_DEPENDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
qt_standard_project_setup()

if(WIN32)
    enable_language(RC)
    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/windows/icon.rc")
endif()

qt_add_executable(Flashcards
    main.cpp
    mainwindow.cpp
    mainwindow.h
    resources.qrc
    snap/snapcraft.yaml
    snap/gui/flashcards-by-ethan.desktop
    README.md
    ${APP_ICON_RESOURCE_WINDOWS}
)

# Windows Icon
if(WIN32 AND APP_ICON_RESOURCE_WINDOWS)
    set_source_files_properties(${APP_ICON_RESOURCE_WINDOWS} PROPERTIES
        OBJECT_DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/Flashcards_autogen/mocs_compilation.cpp"
    )
endif()
if(WIN32)
    set_target_properties(Flashcards PROPERTIES
        OUTPUT_NAME "Flashcards by Ethan"
        WIN32_EXECUTABLE TRUE
    )
else()
    set_target_properties(Flashcards PROPERTIES
        OUTPUT_NAME "flashcards-by-ethan"
        MACOSX_BUNDLE TRUE
    )
endif()

target_link_libraries(Flashcards PRIVATE Qt6::Core Qt6::Widgets)

install(TARGETS Flashcards
    RUNTIME DESTINATION bin
)

install(FILES snap/gui/flashcards-by-ethan.desktop
    DESTINATION share/applications
)

install(FILES icons/icon.png
    DESTINATION share/icons/hicolor/256x256/apps
    RENAME flashcards-by-ethan.png
)

set(CPACK_PACKAGE_NAME "flashcards-by-ethan")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})

set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}_amd64")

set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ethan McCall <ethan-mccall@users.noreply.github.com>")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "A simple flashcard and quiz app built for effective learning.")
set(CPACK_DEBIAN_PACKAGE_SECTION "education")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/ethan-mccall/flashcards-by-ethan")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")

set(CPACK_GENERATOR "DEB")
include(CPack)
