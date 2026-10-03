set(NAME parser.csv)
include(FetchContent)

FetchContent_Declare(
  csv
  GIT_REPOSITORY https://github.com/vincentlaucsb/csv-parser.git
  GIT_SHALLOW TRUE 
  GIT_TAG 2.2.1
  SYSTEM
) 

FetchContent_MakeAvailable(csv)

desbordante_add_lib(NAME OBJECT)
target_sources(${NAME} PRIVATE csv_parser.cpp)
target_link_libraries(${NAME} PRIVATE csv Boost::headers)
