set(NAME fd.pattern_fd_verifier)
desbordante_add_lib(NAME)
target_sources(
    ${NAME}
    PRIVATE pattern_fd_verifier.cpp
            pattern_fd_stats_calculator.cpp
            model/pattern_info.cpp
            model/tokenizer.cpp
)
target_link_libraries(
    ${NAME}
    PUBLIC ${DESBORDANTE_PREFIX}::config
    PRIVATE ${DESBORDANTE_PREFIX}::model::table Boost::headers
)
