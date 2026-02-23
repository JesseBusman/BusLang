#include "proof.h"
#include "id.h"
#include "expression.h"
#include "statement.h"


void Proof_Id::print() const {
	std::print("{}:{}", id.name, id.id);
}


void Proof_Block::print() const {
	std::print("{{\n");
	for (auto& s : statements) {
		s->print(1);
		std::println();
	}
	std::print("    ");
	finalProof->print();
	std::print("\n}}");
}


void Proof_Unwrap::print() const {
	std::print("unwrap (");
	subProof->print();
	std::print(")");
}


void Proof_RawUnwrap::print() const {
	std::print("rawunwrap (");
	subProof->print();
	std::print(")");
}


void Proof_Wrap::print() const {
	std::print("wrap {}:{} (", defId.name, defId.id);
	subProof->print();
	std::print(")");
}

void Proof_RawWrap::print() const {
	std::print("rawwrap {}:{} (", defId.name, defId.id);
	subProof->print();
	std::print(")");
}


void Proof_Shove::print() const {
	std::print("(");
	left->print();
	std::print(") > (");
	right->print();
	std::print(")");
}

void Proof_RawShove::print() const {
	std::print("(");
	left->print();
	std::print(") >> (");
	right->print();
	std::print(")");
}


void Proof_Substitute::print() const {
	std::print("substitute ");
	for (unsigned int i=0; i<vec.size(); i++) {
		if (i != 0) std::print(", ");
		std::print("{}=(", vec[i].first);
		vec[i].second->print();
		std::print(")");
	}
	std::print(" in ");
	subProof->print();
}
