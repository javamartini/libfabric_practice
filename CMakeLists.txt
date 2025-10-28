cmake_minimum_required(VERSION 3.13)
project(libfabric_practice)

# === Find package configuration info. ===
find_package(PkgConfig REQUIRED) # Ensure the 'pkg-config' binary is available.
pkg_check_modules(LIBFABRIC REQUIRED libfabric) # (<prefix>, <arg>, <name>.pc)

set(CMAKE_CXX_STANDARD 20)

# === Target executable. ===
add_executable(libfabric_practice
	#./libfabric_practice/main.cpp
)

# === Include directories. ===
target_include_directories(libfabric_practice
	#./libfabric_practice/include
	${LIBFABRIC_INCLUDE_DIRS}
)

# === Linker directories. ===
target_link_directories(libfabric_practice
	${LIBFABRIC_LIBRARY_DIRS}
)

# === Linker libraries. ===
target_link_libraries(libfabric_practice
	${LIBFABRIC_LIBRARIES}
)
