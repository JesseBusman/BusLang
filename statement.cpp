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
	std::print("define {}:{} ", defId.name, defId.id);
	for (unsigned int i=0; i<varIds.size(); i++) {
		if (i != 0) std::print(", ");
		std::print("{}:{}", varIds[i].name, varIds[i].id);
	}
	std::print(" = (");
	rhs->print();
	std::print(");");
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

