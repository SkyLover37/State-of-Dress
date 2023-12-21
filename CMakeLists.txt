cmake_minimum_required(VERSION 3.21)
message("Using toolchain file ${CMAKE_TOOLCHAIN_FILE}.")

########################################################################################################################
## Define project
########################################################################################################################
project(
        StateOfDress
        VERSION 1.0.0
        DESCRIPTION ""
        LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)

include(GNUInstallDirs)

configure_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.rc.in
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc
        @ONLY)

set(headers
        #src/IPluginInterface.h
        include/SkillExperienceBuffer.h
        src/Animation/TimeBounceInterpolator.h
        src/Animation/TimeColorInterpolator.h
        src/Animation/TimeTrapezoidInterpolator.h
        src/Animation/TimeInterpolator/TimeFloatInterpolator.h
        src/Animation/TimeInterpolator/TimeInterpolatorManager.h
        src/Rendering/Drawer.h
        src/Rendering/RenderManager.h
        src/Rendering/TextureManager.h
        src/utilities/Utils.h
        #src/UserInput/Controls.h
        src/UserInput/Input.h
        src/lib/imgui_freetype.h
        src/lib/nanosvg.h
        src/lib/nanosvgrast.h
        )
set(sources
        src/Main.cpp
        
        src/Animation/TimeBounceInterpolator.cpp
        src/Animation/TimeColorInterpolator.cpp
        src/Animation/TimeTrapezoidInterpolator.cpp
        src/lib/imgui_freetype.cpp
        src/Animation/TimeInterpolator/TimeFloatInterpolator.cpp
        src/Animation/TimeInterpolator/TimeInterpolatorManager.cpp
        src/Rendering/Drawer.cpp
        src/Rendering/RenderManager.cpp
        src/Rendering/TextureManager.cpp
        src/utilities/Utils.cpp
        #src/UserInput/Controls.cpp
        src/UserInput/Input.cpp
        src/SkillExperienceBuffer.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/version.rc)


source_group(
        TREE ${CMAKE_CURRENT_SOURCE_DIR}
        FILES
        ${headers}
        ${sources})

#########################################################################################################################
### Build options
#########################################################################################################################
message("Options:")


########################################################################################################################
## Configure target DLL
########################################################################################################################
find_package(CommonLibSSE CONFIG REQUIRED)

add_commonlibsse_plugin(${PROJECT_NAME} SOURCES ${headers} ${sources})
add_library("${PROJECT_NAME}::${PROJECT_NAME}" ALIAS "${PROJECT_NAME}")
find_package(imgui REQUIRED)
find_package(Freetype REQUIRED)
target_include_directories(${PROJECT_NAME}
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Rendering>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Utilities>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/UserInput>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Animation>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Animation/TimeInteroplator>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Animation/lib>
        $<INSTALL_INTERFACE:src>)
target_link_libraries(${PROJECT_NAME} PUBLIC 
imgui::imgui
Freetype::Freetype)
target_include_directories(${PROJECT_NAME}
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Rendering>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Utilities>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/UserInput>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Animation>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Animation/TimeInteroplator>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src/Animation/lib>
        )

target_precompile_headers(${PROJECT_NAME}
        PRIVATE
        src/PCH.h)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/Sample"
        DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(TARGETS ${PROJECT_NAME}
        DESTINATION "${CMAKE_INSTALL_LIBDIR}")

########################################################################################################################
## Automatic plugin deployment
########################################################################################################################
# Automatic deployment to FOMOD directory.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(BUILD_NAME "Debug")
else()
    set(BUILD_NAME "Release") 
endif()
install(DIRECTORY DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/Papyrus${BUILD_NAME}/")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/Plugin${BUILD_NAME}/")
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/Plugin${BUILD_NAME}/")
if(${CMAKE_BUILD_TYPE} STREQUAL Debug OR ${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${PROJECT_NAME}> "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/Plugin${BUILD_NAME}/")
endif()
file(GLOB_RECURSE OUTPUT_DLLS "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/**/*.dll")
file(GLOB_RECURSE OUTPUT_PDBS "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/**/*.pdb")
file(GLOB_RECURSE OUTPUT_SCRIPTS "${CMAKE_CURRENT_SOURCE_DIR}/contrib/Distribution/**/*.pex")
set_property(TARGET ${PROJECT_NAME}
        APPEND PROPERTY ADDITIONAL_CLEAN_FILES "${OUTPUT_DLLS}")
set_property(TARGET ${PROJECT_NAME}
        APPEND PROPERTY ADDITIONAL_CLEAN_FILES "${OUTPUT_PDBS}")
set_property(TARGET ${PROJECT_NAME}
        APPEND PROPERTY ADDITIONAL_CLEAN_FILES "${OUTPUT_SCRIPTS}")

# Automatic deployment to Mod Organizer 2 mod directory.
foreach(DEPLOY_TARGET $ENV{StateOfDress})
    message("Adding deployment target ${DEPLOY_TARGET}.")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${DEPLOY_TARGET}/SKSE/Plugins/")
    if(${CMAKE_BUILD_TYPE} STREQUAL Debug OR ${CMAKE_BUILD_TYPE} STREQUAL RelWithDebInfo)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_PDB_FILE:${PROJECT_NAME}> "${DEPLOY_TARGET}/SKSE/Plugins/")
    endif()
endforeach()
