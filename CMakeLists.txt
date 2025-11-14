cmake_minimum_required(VERSION 3.31)
project(libfabric_practice CXX)

# === Find package configuration info. ===
find_package(PkgConfig REQUIRED) # Ensure the 'pkg-config' binary is available.
pkg_check_modules(LIBFABRIC REQUIRED libfabric) # (<prefix>, <arg>, <name>.pc)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(SERVER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libfabric_practice/server)
set(CLIENT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libfabric_practice/client)
set(LOCAL_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libfabric_practice/lib)

# === Target executable 'server'. ===
add_executable(server
	${SERVER_DIR}/main.cpp
	${SERVER_DIR}/src/net.cpp
	${LOCAL_LIB_DIR}/src/err.cpp
)

# === 'server' included directories. ===
target_include_directories(server
	PRIVATE ${SERVER_DIR}/include
	PRIVATE ${LOCAL_LIB_DIR}/include
	PRIVATE ${LIBFABRIC_INCLUDE_DIRS}
)

# === 'server' target directories for linker. ===
target_link_directories(server
	PRIVATE ${LIBFABRIC_LIBRARY_DIRS}
)

# === 'server' libraries to be linked. ===
target_link_libraries(server
	${LIBFABRIC_LIBRARIES}
)

# === Target executable 'client'. ===
add_executable(client
	${CLIENT_DIR}/main.cpp
	${CLIENT_DIR}/src/net.cpp
	${LOCAL_LIB_DIR}/src/err.cpp
)

# === 'client' included directories. ===
target_include_directories(client
	PRIVATE ${CLIENT_DIR}/include
	PRIVATE ${LOCAL_LIB_DIR}/include
	PRIVATE ${LIBFABRIC_INCLUDE_DIRS}
)

# === 'client' target directories for linker. ===
target_link_directories(client
	PRIVATE ${LIBFABRIC_LIBRARY_DIRS}
)

# === 'client' libraries to be linked. ===
target_link_libraries(client
	${LIBFABRIC_LIBRARIES}
)
