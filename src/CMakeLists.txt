set(SOURCES 	
    data.cpp
    main.cpp
    play.cpp
    interface.cpp
    playlist.cpp
    log.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -g")

add_executable(${NAME} ${SOURCES})

target_link_libraries(${NAME} ${LIBS})
