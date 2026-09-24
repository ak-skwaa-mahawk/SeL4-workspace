include(${SEL4_TUTORIALS_DIR}/settings.cmake)
sel4_tutorials_regenerate_tutorial(${CMAKE_CURRENT_SOURCE_DIR})

cmake_minimum_required(VERSION 3.10)
project(libraries-3 C ASM)

sel4_tutorials_setup_roottask_tutorial_environment()

# Target 1: Compute Server
add_executable(compute_srv srv.c)
target_link_libraries(compute_srv sel4runtime sel4 muslc utils
                      sel4muslcsys sel4platsupport sel4utils sel4debug)

# Target 2: Client
add_executable(client client.c)
target_link_libraries(client sel4runtime sel4 muslc utils
                      sel4muslcsys sel4platsupport sel4utils sel4debug)

# Bundle both processes into CPIO archive
include(cpio)
set(ARCHIVE_FILES "$<TARGET_FILE:compute_srv>;$<TARGET_FILE:client>")
MakeCPIO(archive.o "${ARCHIVE_FILES}" DEPENDS compute_srv client)

# Root Task
add_executable(libraries-3 archive.o main.c)
target_link_libraries(libraries-3
    sel4 muslc utils sel4tutorials sel4vka
    sel4muslcsys sel4platsupport sel4utils sel4debug sel4allocman)

include(rootserver)
DeclareRootserver(libraries-3)
