# CMake script to embed a shader file as a C++ string literal.

file(READ "${INPUT_FILE}" content)
file(WRITE "${OUTPUT_FILE}"
    "inline constexpr char ${VARIABLE_NAME}[] = R\"glsl(\n${content}\n)glsl\";\n"
)