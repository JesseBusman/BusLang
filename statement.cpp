#include <print>

#include "statement.h"
#include "proof.h"
#include "expression.h"


void printIndent(unsigned int indentLevel) {
	for (unsigned int i=0; i<indentLevel; i++) std::print("    ");
}


void Statement_Block::print(unsigned int indentLevel) const {
	printIndent(indentLevel);
	std::print("{{\n");
	for (auto& s : statements) {
		s->print(indentLevel+1);
		std::println();
	}
	printIndent(indentLevel);
	std::print("}}");
}


void Statement_Print::print(unsigned int indentLevel) const {
	printIndent(indentLevel);
	std::print("print ");
	proof->print();
	std::print(";");
}


void Statement_ForAny::print(unsigned int indentLevel) const {
	printIndent(indentLevel);
	std::print("forany ");
	for (unsigned int i=0; i<varIds.size(); i++) {
		if (i != 0) std::print(", ");
		std::print("{}:{}", varIds[i].name, varIds[i].id);
	}
	std::print(" :");
}


void Statement_Assume::print(unsigned int indentLevel) const {
	printIndent(indentLevel);
	std::print("assume {}:{} proves (", id.name, id.id);
	assumedProposition->print();
	std::print(");");
}


void Statement_Require::print(unsigned int indentLevel) const {
	printIndent(indentLevel);
	std::print("require {}:{} proves (", id.name, id.id);
	requiredProposition->print();
	std::print(") by (");
	proof->print();
	std::print(");");
}


void Statement_Define::print(unsigned int indentLevel) const {
	printIndent(indentLevel);
	std::println("define {}:{} (", defId.name, defId.id);
	for (auto& patternAndValue : patternsAndValues) {
		printIndent(indentLevel+1);
		if (patternAndValue.first.first.size() != 0) {
			std::print("forany ");
			for (unsigned int i=0; i<patternAndValue.first.first.size(); i++) {
				if (i != 0) std::print(", ");
				std::print("{}:{}", patternAndValue.first.first[i].name, patternAndValue.first.first[i].id);
			}
			std::print(": ");
		}
		patternAndValue.first.second->print();
		std::print(" = ");
		patternAndValue.second->print();
		std::println(",");
	}
	printIndent(indentLevel);
	std::println(");");
}


void Statement_Atoms::print(unsigned int indentLevel) const {
	printIndent(indentLevel);
	std::print("atom ");
	for (unsigned int i=0; i<atomIds.size(); i++) {
		if (i != 0) std::print(", ");
		std::print("{}", atomIds[i].name);
	}
	std::println(";");
}


void Statement_MustError::print(unsigned int indentLevel) const {
	printIndent(indentLevel);
	std::print("musterror ");
	proof->print();
	std::print(";");
}

