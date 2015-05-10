cmake_minimum_required(VERSION 2.6)

project (gflags)

add_definitions(-D_UNICODE -DUNICODE)
add_executable (gflags
    gflags.cpp
    console.cpp
    dialog.cpp
    main.cpp
    gflags.h
    resource.h
    gflags.rc
    )

set_target_properties (gflags PROPERTIES
    LINK_FLAGS "/MANIFEST:NO")
