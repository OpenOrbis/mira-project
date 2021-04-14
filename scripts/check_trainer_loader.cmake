message("check_trainer_loader called")

set(TRAINER_LOADER_BIN ${CMAKE_BINARY_DIR}/trainer_loader/libtrainer_loader.so)
if (EXISTS ${TRAINER_LOADER_BIN})
    message("trainer loader found")
    add_compile_definitions(TRAINER_LOADER_BIN_PATH="${TRAINER_LOADER_BIN}")
else()
    message(FATAL_ERROR "could not find built trainer_loader.")
endif()