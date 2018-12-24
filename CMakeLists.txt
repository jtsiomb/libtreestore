cmake_minimum_required(VERSION 3.0)
project(libtreestore)

include(GNUInstallDirs)

set(SO_MAJOR 0)
set(SO_MINOR 2)

file(GLOB src "src/*.c")
file(GLOB hdr "src/*.h")
file(GLOB pubhdr "include/*.h")

if(WIN32)
	set(def treestore.def)
endif()

add_library(treestore SHARED ${src} ${def} ${hdr} ${pubhdr})
add_library(treestore-static STATIC ${src} ${hdr} ${pubhdr})

set_target_properties(treestore PROPERTIES VERSION ${SO_MAJOR}.${SO_MINOR})
set_target_properties(treestore PROPERTIES SOVERSION ${SO_MAJOR})

if(MSVC)
	set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} /wd4244 /wd4996")
endif()

if(NOT WIN32)
	set_target_properties(treestore-static PROPERTIES OUTPUT_NAME treestore)
endif()

target_include_directories(treestore PUBLIC include)
target_include_directories(treestore-static PUBLIC include)

target_link_libraries(treestore)
target_link_libraries(treestore-static)

install(TARGETS treestore
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(TARGETS treestore-static
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES ${pubhdr} DESTINATION include)
