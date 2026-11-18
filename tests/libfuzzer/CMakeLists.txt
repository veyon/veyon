# coverage feedback from the code under test, not only from the harnesses
target_compile_options(veyon-core PRIVATE -fsanitize=fuzzer-no-link)

add_subdirectory(core)
