set(NAME util)
desbordante_add_lib(NAME OBJECT)
target_sources(
    ${NAME} PRIVATE convex_hull.cpp create_dd.cpp levenshtein_distance.cpp qgram_vector.cpp
                    worker_thread_pool.cpp
)
target_link_libraries(${NAME} PUBLIC magic_enum::magic_enum)
target_link_libraries(${NAME} PRIVATE spdlog::spdlog_header_only Boost::headers)
