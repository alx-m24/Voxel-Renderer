const char* postProcessing_vert =
"#version 430 core\n"
"layout (location = 0) in vec3 aPos;\n"
"\n"
"void main() {\n"
"	gl_Position = vec4(aPos, 1.0);\n"
"}\n"
;
