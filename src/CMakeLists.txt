set(SOURCES 	
		 data.cpp
		 main.cpp
		 play.cpp
		 decode.cpp
		 interface.cpp
		 playlist.cpp)

set(LIBS 	
		ncurses
		avcodec
		avformat
		avutil
		ao
		pthread)

set(FLAGS
		-std=c++14)

find_package(Threads)

add_definitions(${FLAGS})

add_executable(${NAME} ${SOURCES})

target_link_libraries(${NAME} ${LIBS})
