set(NAME util)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME}
    PRIVATE convex_hull.cpp create_dd.cpp levenshtein_distance.cpp progress.cpp qgram_vector.cpp
            worker_thread_pool.cpp
    PUBLIC FILE_SET HEADERS BASE_DIRS "${PROJECT_SOURCE_DIR}/src"
)
target_link_libraries(${NAME} PRIVATE spdlog::spdlog_header_only better-enums Boost::headers)
