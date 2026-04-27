cmake_minimum_required(VERSION 3.22)
project(Flashcards VERSION 0.1.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
qt_standard_project_setup()

qt_add_executable(Flashcards
    main.cpp
    mainwindow.cpp
    mainwindow.h
    resources.qrc
    snap/snapcraft.yaml
    snap/gui/flashcards-by-ethan.desktop
)

set_target_properties(Flashcards PROPERTIES
    OUTPUT_NAME "flashcards-by-ethan"
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

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
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ethan McCall <your-email@example.com>")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION "A simple flashcard and quiz app built for effective learning.")
set(CPACK_DEBIAN_PACKAGE_SECTION "education")
set(CPACK_DEBIAN_PACKAGE_HOMEPAGE "https://github.com/ethan-mccall/flashcards-by-ethan")
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ON)
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}_${CPACK_PACKAGE_VERSION}-1_amd64")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
include(CPack)
