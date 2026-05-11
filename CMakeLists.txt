cmake_minimum_required(VERSION 3.13)

set(PROJECT_NAME "VTK-Workbench")
set(WINDOW_WIDTH 960)
set(WINDOW_HEIGHT 540)

# C++ 17
set(CMAKE_CXX_STANDARD 17)

# 64-bit WASM memory model and multi-threading with pthread
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -sMEMORY64=1 -pthread")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sMEMORY64=1 -pthread")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sMEMORY64=1 -pthread")
# set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} -sMEMORY64=1 -pthread")

# Set project
project(${PROJECT_NAME})

# EMSCRIPTEN only
if (NOT EMSCRIPTEN)
  message("Skipping example: This needs to run inside an Emscripten build environment")
  return ()
endif ()

# Set VTK dependency
find_package(VTK
  COMPONENTS
    # FiltersSources      # VTK pipeline
    InteractionStyle    # Mouse handling
    RenderingOpenGL2    # For Rendering
    RenderingVolume
    RenderingVolumeOpenGL2
    # RenderingUI         # For SDL2 Window
    RenderingAnnotation
    FiltersFlowPaths
    InteractionWidgets  # For OrientationMarkerWidget
)

if (NOT VTK_FOUND)
  message("Skipping example: ${VTK_NOT_FOUND_MESSAGE}")
  return ()
endif ()

# ============================================================================
# Voro++ Configuration
# ============================================================================
set(VOROPP_DIR "${CMAKE_SOURCE_DIR}/webassembly/dependencies/voro++")
set(VOROPP_INCLUDE_DIR "${VOROPP_DIR}/include/voro++")
set(VOROPP_LIB_DIR "${VOROPP_DIR}/lib")
set(VOROPP_LIBRARY "${VOROPP_LIB_DIR}/libvoro++.a")

# ============================================================================
# spglib Configuration
# ============================================================================
# set(SPGLIB_DIR "${CMAKE_SOURCE_DIR}/webassembly/dependencies/spglib")
# set(SPGLIB_INCLUDE_DIR "${SPGLIB_DIR}/include")
# set(SPGLIB_LIB_DIR "${SPGLIB_DIR}/lib")
# set(SPGLIB_LIBRARY "${SPGLIB_LIB_DIR}/libsymspg.a")

# Source list (Phase 1 bootstrap)
set(SOURCES_APP
  webassembly/src/app/app.cpp
  webassembly/src/app/app.h
)

set(SOURCES_CORE
  # vtk
  webassembly/src/core/vtk/vtk_viewer.cpp
  webassembly/src/core/vtk/vtk_viewer.h
  webassembly/src/core/vtk/mouse_interactor.cpp
  webassembly/src/core/vtk/mouse_interactor.h
  webassembly/src/core/vtk/batch_update_system.cpp
  webassembly/src/core/vtk/batch_update_system.h
  # io
  webassembly/src/core/io/file_dialog.cpp
  webassembly/src/core/io/file_dialog.h
  webassembly/src/core/io/format_registry.cpp
  webassembly/src/core/io/format_registry.h
  webassembly/src/core/io/xsf_parser.cpp
  webassembly/src/core/io/xsf_parser.h
  webassembly/src/core/io/chgcar_parser.cpp
  webassembly/src/core/io/chgcar_parser.h
  webassembly/src/core/io/rho_parser.cpp
  webassembly/src/core/io/rho_parser.h
  webassembly/src/core/io/unv_reader.cpp
  webassembly/src/core/io/unv_reader.h
  # data
  webassembly/src/core/data/element_database.cpp
  webassembly/src/core/data/element_database.h
  webassembly/src/core/data/colormap.cpp
  webassembly/src/core/data/colormap.h
  webassembly/src/core/data/lcrs_tree.cpp
  webassembly/src/core/data/lcrs_tree.h
  webassembly/src/core/data/string_utils.cpp
  webassembly/src/core/data/string_utils.h
  webassembly/src/core/data/color.h
  # scene
  webassembly/src/core/scene/scene_state.cpp
  webassembly/src/core/scene/scene_state.h
  webassembly/src/core/scene/structure_registry.cpp
  webassembly/src/core/scene/structure_registry.h
  webassembly/src/core/scene/selection.cpp
  webassembly/src/core/scene/selection.h
  webassembly/src/core/scene/hover.cpp
  webassembly/src/core/scene/hover.h
  webassembly/src/core/scene/events.h
  # render
  webassembly/src/core/render/font_manager.cpp
  webassembly/src/core/render/font_manager.h
  webassembly/src/core/render/image.cpp
  webassembly/src/core/render/image.h
  webassembly/src/core/render/texture.cpp
  webassembly/src/core/render/texture.h
  # ui
  webassembly/src/core/ui/widgets.cpp
  webassembly/src/core/ui/widgets.h
  webassembly/src/core/ui/ui_color_utils.h
)

set(SOURCES_FEATURES
  # Phase 3.1: Utilities / Brillouin Zone
  webassembly/src/features/utilities/brillouin_zone/bz_plot.cpp
  webassembly/src/features/utilities/brillouin_zone/bz_plot.h
  webassembly/src/features/utilities/brillouin_zone/special_points.h
  webassembly/src/features/utilities/brillouin_zone/bz_plot_layer.cpp
  webassembly/src/features/utilities/brillouin_zone/bz_plot_layer.h
  webassembly/src/features/utilities/brillouin_zone/bz_plot_controller.cpp
  webassembly/src/features/utilities/brillouin_zone/bz_plot_controller.h
  webassembly/src/features/utilities/brillouin_zone/bz_plot_ui.cpp
  webassembly/src/features/utilities/brillouin_zone/bz_plot_ui.h
  webassembly/src/features/utilities/brillouin_zone/bz_menu.cpp
  webassembly/src/features/utilities/brillouin_zone/bz_menu.h

  # Phase 3.2: Data / Charge Density + Slice
  webassembly/src/features/data/data_menu.cpp
  webassembly/src/features/data/data_menu.h
  webassembly/src/features/data/charge_density/charge_density.cpp
  webassembly/src/features/data/charge_density/charge_density.h
  webassembly/src/features/data/charge_density/isosurface_renderer.cpp
  webassembly/src/features/data/charge_density/isosurface_renderer.h
  webassembly/src/features/data/charge_density/volume_renderer.cpp
  webassembly/src/features/data/charge_density/volume_renderer.h
  webassembly/src/features/data/charge_density/charge_density_controller.cpp
  webassembly/src/features/data/charge_density/charge_density_controller.h
  webassembly/src/features/data/charge_density/charge_density_ui.cpp
  webassembly/src/features/data/charge_density/charge_density_ui.h
  webassembly/src/features/data/slice/slice_renderer.cpp
  webassembly/src/features/data/slice/slice_renderer.h
  webassembly/src/features/data/slice/slice_controller.cpp
  webassembly/src/features/data/slice/slice_controller.h
  webassembly/src/features/data/slice/slice_ui.cpp
  webassembly/src/features/data/slice/slice_ui.h

  # Phase 3.3: Build / Periodic Table + Bravais Lattice
  webassembly/src/features/build/build_menu.cpp
  webassembly/src/features/build/build_menu.h
  webassembly/src/features/build/periodic_table/periodic_table.cpp
  webassembly/src/features/build/periodic_table/periodic_table.h
  webassembly/src/features/build/periodic_table/periodic_table_controller.cpp
  webassembly/src/features/build/periodic_table/periodic_table_controller.h
  webassembly/src/features/build/periodic_table/periodic_table_ui.cpp
  webassembly/src/features/build/periodic_table/periodic_table_ui.h
  webassembly/src/features/build/bravais/crystal_structure.cpp
  webassembly/src/features/build/bravais/crystal_structure.h
  webassembly/src/features/build/bravais/crystal_system.cpp
  webassembly/src/features/build/bravais/crystal_system.h
  webassembly/src/features/build/bravais/bravais_controller.cpp
  webassembly/src/features/build/bravais/bravais_controller.h
  webassembly/src/features/build/bravais/bravais_lattice_ui.cpp
  webassembly/src/features/build/bravais/bravais_lattice_ui.h

  # Phase 3.4.1: Edit / Cell
  webassembly/src/features/edit/edit_menu.cpp
  webassembly/src/features/edit/edit_menu.h
  webassembly/src/features/edit/cell/cell_manager.cpp
  webassembly/src/features/edit/cell/cell_manager.h
  webassembly/src/features/edit/cell/cell_renderer.cpp
  webassembly/src/features/edit/cell/cell_renderer.h
  webassembly/src/features/edit/cell/cell_controller.cpp
  webassembly/src/features/edit/cell/cell_controller.h
  webassembly/src/features/edit/cell/cell_info_ui.cpp
  webassembly/src/features/edit/cell/cell_info_ui.h

  # Phase 3.4.2: Edit / Atoms
  webassembly/src/features/edit/atoms/atom_manager.cpp
  webassembly/src/features/edit/atoms/atom_manager.h
  webassembly/src/features/edit/atoms/surrounding_atom_manager.cpp
  webassembly/src/features/edit/atoms/surrounding_atom_manager.h
  webassembly/src/features/edit/atoms/atom_renderer.cpp
  webassembly/src/features/edit/atoms/atom_renderer.h
  webassembly/src/features/edit/atoms/atoms_controller.cpp
  webassembly/src/features/edit/atoms/atoms_controller.h
  webassembly/src/features/edit/atoms/atom_editor_ui.cpp
  webassembly/src/features/edit/atoms/atom_editor_ui.h

  # Phase 3.4.3: Edit / Bonds
  webassembly/src/features/edit/bonds/bond_manager.cpp
  webassembly/src/features/edit/bonds/bond_manager.h
  webassembly/src/features/edit/bonds/bond_renderer.cpp
  webassembly/src/features/edit/bonds/bond_renderer.h
  webassembly/src/features/edit/bonds/bonds_controller.cpp
  webassembly/src/features/edit/bonds/bonds_controller.h
  webassembly/src/features/edit/bonds/bond_ui.cpp
  webassembly/src/features/edit/bonds/bond_ui.h
)

add_executable(${PROJECT_NAME}
  webassembly/src/main.cpp
  webassembly/src/bind_function.cpp
  ${SOURCES_APP}
  ${SOURCES_CORE}
  ${SOURCES_FEATURES}
)

# Safety belt: fail configuration if any legacy source enters the target.
get_target_property(_TARGET_SRCS ${PROJECT_NAME} SOURCES)
foreach(_src IN LISTS _TARGET_SRCS)
  if(_src MATCHES "webassembly/src/legacy/")
    message(FATAL_ERROR "Phase 1 violation: legacy source in target: ${_src}")
  endif()
endforeach()

# Add dependencies
set(DEP_INCLUDE_DIR)
set(DEP_LIB_DIR)
set(DEP_LIBS)

include(Dependency.cmake)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${DEP_INCLUDE_DIR}
    ${VOROPP_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}/webassembly/src
    ${CMAKE_SOURCE_DIR}/webassembly/src/legacy
    # ${SPGLIB_INCLUDE_DIR}
)
target_link_directories(${PROJECT_NAME}
  PRIVATE
    ${DEP_LIB_DIR}
    # ${SPGLIB_LIBRARY}
)
target_link_libraries(${PROJECT_NAME}
  PRIVATE
    ${DEP_LIBS}
    ${VTK_LIBRARIES}
    ${VOROPP_LIBRARY}
    ${CMAKE_SOURCE_DIR}/webassembly/dependencies/VTK/Release/lib/libvtkRenderingVolumeOpenGL2.a
)

# Declare emscripten option variables
set(EMSCRIPTEN_COMPILE_OPTIONS)
set(EMSCRIPTEN_LINK_OPTIONS)
set(EMSCRIPTEN_OPTIMIZATIONS)
set(EMSCRIPTEN_DEBUG_OPTIONS)

list(APPEND EMSCRIPTEN_COMPILE_OPTIONS
  "SHELL:-flto"
  "SHELL:-sSHARED_MEMORY=1"
)

list(APPEND EMSCRIPTEN_LINK_OPTIONS
  "SHELL:--bind"
  "SHELL:-lidbfs.js"                # IndexedDB file system support
  "SHELL:-lworkerfs.js"             # Worker file system support
  "SHELL:-sEXPORTED_FUNCTIONS=['_main','_malloc','_free']"
  "SHELL:-sEXPORTED_RUNTIME_METHODS=['FS','WORKERFS']"
  "SHELL:-sWASM=1"
  "SHELL:-sENVIRONMENT=web,worker"
  "SHELL:-sUSE_WEBGL2=1"
  "SHELL:-sMODULARIZE=1"
  "SHELL:-sEXPORT_NAME='createViewer'"
  "SHELL:-sUSE_GLFW=3"
  "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/webassembly/resources/@/resources/"
  # "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/webassembly/src/legacy/imgui.ini@/defaults/imgui.ini"
  "SHELL:-sINITIAL_MEMORY=256MB"
  "SHELL:-sALLOW_MEMORY_GROWTH=1"
  "SHELL:-sMAXIMUM_MEMORY=16GB"
  "SHELL:-sUSE_PTHREADS=1" 
  "SHELL:-sPTHREAD_POOL_SIZE=5"
  "SHELL:-sNO_EXIT_RUNTIME=1"
  "SHELL:-Wno-pthreads-mem-growth"  # Suppress warning (-Wpthreads-mem-growth)
  "SHELL:-sWASM_BIGINT=1"           # Use BigInt for 64-bit integers
  "SHELL:-sSTACK_SIZE=1MB"          # Default stack size is 64KB.
  "SHELL:-flto"                     # Link time optimization
  "SHELL:-sSHARED_MEMORY=1"         # Use shared memory for multi-threading
)

target_compile_definitions(${PROJECT_NAME}
  PRIVATE
    WINDOW_NAME="${WINDOW_NAME}"
    WINDOW_WIDTH=${WINDOW_WIDTH}
    WINDOW_HEIGHT=${WINDOW_HEIGHT}
    SHOW_IMGUI_DEMO  # Show ImGui demo window. Comment this for production build
    # SHOW_FONT_ICONS  # Show font icons. Comment this for production build
)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "Build type: Release mode")
  list(APPEND EMSCRIPTEN_OPTIMIZATIONS "-O3")
  list(APPEND EMSCRIPTEN_DEBUG_OPTIONS "-g0")
  # list(APPEND EMSCRIPTEN_LINK_OPTIONS
  #   "SHELL:--memoryprofiler"  # If this option turns on, resizing does not work properly.
  # )
elseif (CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Build type: Debug mode")
  list(APPEND EMSCRIPTEN_OPTIMIZATIONS "-O0")
  list(APPEND EMSCRIPTEN_DEBUG_OPTIONS "-g3")
  list(APPEND EMSCRIPTEN_LINK_OPTIONS
    "SHELL:-sASSERTIONS=1"
    # "SHELL:--memoryprofiler"  # If this option turns on, resizing does not work properly.
    "SHELL:-sSAFE_HEAP=0"
  )
  target_compile_definitions(${PROJECT_NAME}
    PRIVATE
      DEBUG_BUILD
  )
endif()

target_compile_options(${PROJECT_NAME}
  PRIVATE
    ${EMSCRIPTEN_COMPILE_OPTIONS}
    ${EMSCRIPTEN_OPTIMIZATIONS}
    ${EMSCRIPTEN_DEBUG_OPTIONS}
)

target_link_options(${PROJECT_NAME}
  PRIVATE
    ${EMSCRIPTEN_LINK_OPTIONS}
    ${EMSCRIPTEN_OPTIMIZATIONS}
    ${EMSCRIPTEN_DEBUG_OPTIONS}
)

# Copy wasm files to public folder
if (EMSCRIPTEN)
  install(
    FILES
      ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.data
      ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.wasm
      ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.js
    DESTINATION "${CMAKE_SOURCE_DIR}/public/wasm"
  ) 
endif()

# if DEP_LIST is not empty, add dependencies
if (DEP_LIST)
  add_dependencies(${PROJECT_NAME} ${DEP_LIST})
endif()

# VTK modules initialization
vtk_module_autoinit(
  TARGETS ${PROJECT_NAME}
  MODULES ${VTK_LIBRARIES}
)

