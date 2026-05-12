# CMake script to embed a shader file as a C++ string literal.

file(READ "${INPUT_FILE}" content)

string(TOUPPER "${VARIABLE_NAME}" guard_base)
string(REGEX REPLACE "[^A-Z0-9]" "_" guard "${guard_base}")

file(WRITE "${OUTPUT_FILE}"
    "#ifndef NAG_SHADERS_${guard}_H_\n"
    "#define NAG_SHADERS_${guard}_H_\n"
    "\n"
    "inline constexpr char ${VARIABLE_NAME}[] = R\"glsl(\n${content}\n)glsl\";\n"
    "\n"
    "#endif // NAG_SHADERS_${guard}_H_\n"
)