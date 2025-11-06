cmake_minimum_required(VERSION 3.20)
project(libfabric_practice)

# === Find package configuration info. ===
find_package(PkgConfig REQUIRED) # Ensure the 'pkg-config' binary is available.
pkg_check_modules(LIBFABRIC REQUIRED libfabric) # (<prefix>, <arg>, <name>.pc)
find_package(Threads REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libfabric_practice)

# === Target executable. ===
add_executable(libfabric_practice
	${SRC_DIR}/main.cpp
	${SRC_DIR}/src/net.cpp
)

# === Include directories. ===
target_include_directories(libfabric_practice
	PRIVATE ${SRC_DIR}/include
	PRIVATE ${LIBFABRIC_INCLUDE_DIRS}
)

# === Linker directories. ===
target_link_directories(libfabric_practice
	PRIVATE ${LIBFABRIC_LIBRARY_DIRS}
)

# === Linker libraries. ===
target_link_libraries(libfabric_practice
	${LIBFABRIC_LIBRARIES}
	Threads::Threads
)
