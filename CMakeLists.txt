cmake_minimum_required(VERSION 4.4.2)

# https://github.com/Kitware/CMake/blob/v4.4.2/Help/dev/experimental.rst
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD f35a9ac6-8463-4d38-8eec-5d6008153e7d)

project(buslang CXX)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_SCAN_FOR_MODULES ON)
set(CMAKE_CXX_MODULE_STD ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(a.out)

target_sources(a.out
	PRIVATE
		FILE_SET CXX_MODULES FILES
			file_range.cppm
			expression.cppm
			namespace.cppm
			buslang.cppm
			pattern_matching.cppm
			id.cppm
			parser.cppm
			proofsyntax.cppm
			statement_proof.cppm
			syntax_error.cppm
			syntax.cppm
			evaluate_proof.cppm
)

target_sources(a.out
	PRIVATE
		parse_expression.cpp
		parse_statements.cpp
		parse_proof.cpp
		parse_proofsyntax.cpp
		main.cpp
)
