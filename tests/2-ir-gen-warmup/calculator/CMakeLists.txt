flex_target(calc_lex calculator.l ${CMAKE_CURRENT_BINARY_DIR}/calc_lex.c)
bison_target(calc_syntax calculator.y
  ${CMAKE_CURRENT_BINARY_DIR}/calc_syntax.c
  DEFINES_FILE ${PROJECT_BINARY_DIR}/calculator.h)
add_flex_bison_dependency(calc_lex calc_syntax)
add_library(calc_syntax STATIC
  ${BISON_calc_syntax_OUTPUTS}
  ${FLEX_calc_lex_OUTPUTS}
)
add_executable(
    calc
    calc.cpp
    calc_ast.cpp
    calc_builder.cpp
)
target_link_libraries(
    calc
    IR_lib
    calc_syntax
    common
)
