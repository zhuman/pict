add_test(
  NAME run_pict
  COMMAND perl test.pl $<TARGET_FILE:pict_cli> $<IF:$<CONFIG:Debug>,dbg,rel>.log
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
  NAME run_pict_benchmark
  COMMAND $<TARGET_FILE:pict_benchmark> --runs 3 --benchmark 3^4
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

add_executable(exclusion_allocator_test exclusion_allocator.cpp)
target_link_libraries(exclusion_allocator_test PRIVATE pict_api PictBuildOptions)
add_test(NAME exclusion_allocator COMMAND exclusion_allocator_test)

add_executable(trie_test trie.cpp)
target_link_libraries(trie_test PRIVATE pict_api PictBuildOptions)
add_test(NAME trie COMMAND trie_test)
