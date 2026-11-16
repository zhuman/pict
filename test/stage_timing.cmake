set(model "${CMAKE_CURRENT_BINARY_DIR}/stage-timing-model.txt")
file(WRITE "${model}" [=[A: 0, 1
B: 0, 1
C: 0, 1
{A, B} @ 2
{B, C} @ 2
IF [A] = 0 THEN [C] <> 0;
]=])

execute_process(
  COMMAND "${PICT_EXECUTABLE}" "${model}" /r:17 /t:1
  RESULT_VARIABLE result
  OUTPUT_VARIABLE output
  ERROR_VARIABLE diagnostics
)
if(NOT result EQUAL 0)
  message(FATAL_ERROR "Generation failed: ${diagnostics}")
endif()
if(output MATCHES "\\[stage\\]")
  message(FATAL_ERROR "Stage diagnostics contaminated stdout")
endif()

set(timestamp "\\[[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]T[0-9][0-9]:[0-9][0-9]:[0-9][0-9][.][0-9][0-9][0-9]Z\\]")
foreach(stage tool.execute model.parse constraints.convert exclusions.derive
              generation.submodel generation.root combinations.create
              exclusions.apply generation.rows results.format results.write)
  string(REPLACE "." "\\." pattern "${stage}")
  if(NOT diagnostics MATCHES "${timestamp} \\[stage\\] BEGIN ${pattern} thread=[^ \r\n]+")
    message(FATAL_ERROR "Missing timestamped stage start for ${stage}: ${diagnostics}")
  endif()
  if(NOT diagnostics MATCHES "${timestamp} \\[stage\\] END ${pattern} thread=[^ \r\n]+( item=[0-9]+)? elapsed_ms=[0-9]+[.][0-9][0-9][0-9]")
    message(FATAL_ERROR "Missing elapsed stage time for ${stage}: ${diagnostics}")
  endif()
endforeach()
if(NOT diagnostics MATCHES "BEGIN generation\\.submodel thread=[^ \r\n]+ item=2")
  message(FATAL_ERROR "Submodel timings are not individually identified")
endif()
