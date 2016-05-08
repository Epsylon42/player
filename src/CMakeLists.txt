set(SOURCES 	
		 data.cpp
		 main.cpp
		 play.cpp
		 decode.cpp
		 interface.cpp
         playlist.cpp
         log.cpp)

set(LIBS 	
		ncursesw
		avcodec
		avformat
		avutil
		ao
		pthread)

set(FLAGS
		-std=c++14
		-g)

find_package(Threads)

add_definitions(${FLAGS})

add_executable(${NAME} ${SOURCES})

target_link_libraries(${NAME} ${LIBS})
