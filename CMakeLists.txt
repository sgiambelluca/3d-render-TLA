# Minimum required version fo CMake. Can be modified at your own risk.
# @see https://cmake.org/download/
cmake_minimum_required(VERSION 3.31.6)

# Name of the project and language to use (C, in this case).
project(Flex-Bison-Compiler C)

# Selects the best strategy according to the available compiler in the system.
# @see https://cmake.org/cmake/help/latest/variable/CMAKE_LANG_COMPILER_ID.html
if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
	message(NOTICE "The C compiler is GCC.")

	# Compiles the parser with Bison, generating counter-examples in case S/R or R/R conflicts exists.
	add_custom_command(
		OUTPUT ../src/main/c/frontend/syntactic-analysis/BisonParser.c ../src/main/c/frontend/syntactic-analysis/BisonParser.h
		COMMAND ../src/main/bash/bison.sh
	)

	# Options for GCC.
	add_compile_options(-fsanitize=address)
	add_compile_options(-O3)
	add_compile_options(-static-libgcc)
	add_compile_options(-std=gnu99)
	add_link_options(-fsanitize=address)

	# Compiles the scanner with Flex.
	add_custom_command(
		OUTPUT ../src/main/c/frontend/lexical-analysis/FlexScanner.c ../src/main/c/frontend/lexical-analysis/FlexScanner.h
		COMMAND ../src/main/bash/flex.sh
		DEPENDS ../src/main/c/frontend/syntactic-analysis/BisonParser.c ../src/main/c/frontend/syntactic-analysis/BisonParser.h
	)

	# Embeds the JavaScript runtime in the binary, as a null-terminated string. It's generated again
	# when the runtime changes, because that file is a dependency of the configuration.
	set(RUNTIME_SOURCE ${CMAKE_SOURCE_DIR}/src/main/javascript/Runtime.js)
	set(RUNTIME_OUTPUT ${CMAKE_BINARY_DIR}/generated/Runtime.c)
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${RUNTIME_SOURCE})
	file(READ ${RUNTIME_SOURCE} RUNTIME_HEXADECIMAL HEX)
	string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1, " RUNTIME_BYTES "${RUNTIME_HEXADECIMAL}")
	file(WRITE ${RUNTIME_OUTPUT} "const char runtime[] = { ${RUNTIME_BYTES}0x00 };\n")
	# The bytes above 0x7F (non-ASCII characters) are valid, even if a "char" is signed.
	set_source_files_properties(${RUNTIME_OUTPUT} PROPERTIES COMPILE_OPTIONS -Wno-overflow)

	# Defines the entry-point of the application, and the source-codes (*.c extension).
	# The header files (*.h extension), are automatically included from the source-codes.
	add_executable(Flex-Bison-Compiler
		${RUNTIME_OUTPUT}
		src/main/c/backend/code-generation/Generator.c
		src/main/c/backend/semantic-analysis/SemanticAnalyzer.c
		src/main/c/backend/semantic-analysis/SymbolTable.c
		src/main/c/EntryPoint.c
		src/main/c/frontend/Frontend.c
		src/main/c/frontend/lexical-analysis/FlexActions.c
		src/main/c/frontend/lexical-analysis/FlexScanner.c
		src/main/c/frontend/syntactic-analysis/AbstractSyntaxTree.c
		src/main/c/frontend/syntactic-analysis/BisonActions.c
		src/main/c/frontend/syntactic-analysis/BisonParser.c
		src/main/c/support/configuration/Environment.c
		src/main/c/support/language/String.c
		src/main/c/support/logging/Logger.c
		# Add more *.c files if needed (otherwise, they won't be compiled).
		# ...
	)

	# Link final project and libraries.
	target_link_libraries(Flex-Bison-Compiler m)
else ()
	message(NOTICE "The C compiler is unknown.")
endif ()
