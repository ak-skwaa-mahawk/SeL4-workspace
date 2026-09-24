cmake_minimum_required(VERSION 3.7.2)

project(multiserver C)

set(CMAKE_EXECUTABLE_SUFFIX ".elf")

# Target 1: Compute Server
add_executable(compute_srv src/srv.c)
target_link_libraries(compute_srv sel4 muslc utils)

# Target 2: Client
add_executable(client src/client.c)
target_link_libraries(client sel4 muslc utils)

# Bundle both processes into CPIO archive
MakeCPIO("archive.o" "${CMAKE_CURRENT_BINARY_DIR}/archive.cpio"
    FILES
        "$<TARGET_FILE:compute_srv>"
        "$<TARGET_FILE:client>"
    DEPENDS
        compute_srv
        client
)

# Root Task
add_executable(multiserver src/main.c)
target_link_libraries(multiserver
    sel4
    muslc
    sel4vka
    sel4allocman
    sel4utils
    sel4simple
    sel4simple-default
    sel4platsupport
    utils
    cpio
)
target_sources(multiserver PRIVATE archive.o)

DeclareRootserver(multiserver)
