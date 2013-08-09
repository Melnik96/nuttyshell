project(nuttyshell)

add_library(nuttyshell MODULE nuttyshell.c)

set(EXECUTABLE_OUTPUT_PATH ./bin)
set(LIBRARY_OUTPUT_PATH /bin)

install(TARGETS nuttyshell RUNTIME DESTINATION bin)
