export module BusLang:Statement_Proof;

import std;
import :Expression;
import :Id;

using std::shared_ptr;
using std::pair;
using std::vector;
using std::string_view;

inline void printIndent(unsigned int indentLevel) {
	for (unsigned int i=0; i<indentLevel; i++) std::print("    ");
}

export {

struct Statement;

struct Proof : std::enable_shared_from_this<Proof> {
	FileRange fileRange;
	constexpr Proof(FileRange _fileRange): fileRange(_fileRange) { }
	virtual void print() const = 0;
	virtual ~Proof() = default;
};


struct Proof_Id final : Proof {
	Id id;
	constexpr Proof_Id(FileRange _fileRange, Id _id): Proof(_fileRange), id(_id) { }
	void print() const override;
	inline ~Proof_Id() override = default;
};


struct Proof_Block final : Proof {
	vector<shared_ptr<const Statement>> statements;
	shared_ptr<const Proof> finalProof;
	constexpr Proof_Block(FileRange _fileRange, vector<shared_ptr<const Statement>>&& _statements, shared_ptr<const Proof>&& _finalProof):
		Proof(_fileRange), statements(std::move(_statements)), finalProof(std::move(_finalProof)) { }
	void print() const override;
	inline ~Proof_Block() override = default;
};


struct Proof_Shove final : Proof {
	shared_ptr<const Proof> left;
	shared_ptr<const Proof> right;
	constexpr Proof_Shove(FileRange _fileRange, shared_ptr<const Proof>&& _left, shared_ptr<const Proof>&& _right):
		Proof(_fileRange), left(std::move(_left)), right(std::move(_right))
	{
	}
	void print() const override;
	inline ~Proof_Shove() override = default;
};

struct Proof_RawShove final : Proof {
	shared_ptr<const Proof> left;
	shared_ptr<const Proof> right;
	constexpr Proof_RawShove(FileRange _fileRange, shared_ptr<const Proof>&& _left, shared_ptr<const Proof>&& _right):
		Proof(_fileRange), left(std::move(_left)), right(std::move(_right))
	{
	}
	void print() const override;
	inline ~Proof_RawShove() override = default;
};


struct Proof_Substitute final : Proof {
	vector<pair<string_view, shared_ptr<const Expression>>> vec;
	shared_ptr<const Proof> subProof;
	constexpr Proof_Substitute(FileRange _fileRange, vector<pair<string_view, shared_ptr<const Expression>>>&& _vec, shared_ptr<const Proof>&& _subProof):
		Proof(_fileRange), vec(std::move(_vec)), subProof(std::move(_subProof)) { }
	void print() const override;
	inline ~Proof_Substitute() override = default;
};


struct Proof_TryList final : Proof {
	vector<shared_ptr<const Proof>> tryList;
	constexpr Proof_TryList(FileRange _fileRange, vector<shared_ptr<const Proof>>&& _tryList):
		Proof(_fileRange), tryList(std::move(_tryList))
	{
	}
	void print() const override;
	inline ~Proof_TryList() override = default;
};








struct Statement {
	virtual void print(unsigned int indentLevel) const = 0;
	virtual ~Statement() = default;
};


struct Statement_Block final : Statement {
	vector<shared_ptr<const Statement>> statements;
	constexpr Statement_Block(vector<shared_ptr<const Statement>>&& _statements):
		statements(std::move(_statements)) { }
	void print(unsigned int indentLevel) const override {
		printIndent(indentLevel);
		std::print("{{\n");
		for (auto& s : statements) {
			s->print(indentLevel+1);
			std::println();
		}
		printIndent(indentLevel);
		std::print("}}");
	}
	~Statement_Block() override = default;
};


struct Statement_Print final : Statement {
	shared_ptr<const Proof> proof;
	constexpr Statement_Print(shared_ptr<const Proof>&& _proof):
		proof(std::move(_proof)) { }
	void print(unsigned int indentLevel) const override {
		printIndent(indentLevel);
		std::print("print ");
		proof->print();
		std::print(";");
	}
	~Statement_Print() override = default;
};


struct Statement_ForAny final : Statement {
	vector<Id> varIds; 
	constexpr Statement_ForAny(vector<Id>&& _varIds):
		varIds(std::move(_varIds)) { }
	void print(unsigned int indentLevel) const override {
		printIndent(indentLevel);
		std::print("forany ");
		for (unsigned int i=0; i<varIds.size(); i++) {
			if (i != 0) std::print(", ");
			std::print("{}:{}", varIds[i].name, varIds[i].id);
		}
		std::print(" :");
	}
	~Statement_ForAny() override = default;
};


struct Statement_Assume final : Statement {
	Id id;
	shared_ptr<const Expression> assumedProposition;
	constexpr Statement_Assume(Id _id, shared_ptr<const Expression>&& _assumedProposition):
		id(_id), assumedProposition(std::move(_assumedProposition)) { }
	void print(unsigned int indentLevel) const override {
		printIndent(indentLevel);
		std::print("assume {}:{} proves (", id.name, id.id);
		assumedProposition->print();
		std::print(");");
	}
	~Statement_Assume() override = default;
};


struct Statement_SyntaxAssume final : Statement {
	Id id;
	shared_ptr<const Expression> assumedProposition;
	constexpr Statement_SyntaxAssume(Id _id, shared_ptr<const Expression>&& _assumedProposition):
		id(_id), assumedProposition(std::move(_assumedProposition)) { }
	void print(unsigned int indentLevel) const override {
		printIndent(indentLevel);
		std::print("syntaxassume {}:{} proves (", id.name, id.id);
		assumedProposition->print();
		std::print(");");
	}
	~Statement_SyntaxAssume() override = default;
};


struct Statement_Require final : Statement {
	Id id;
	shared_ptr<const Expression> requiredProposition;
	shared_ptr<const Proof> proof;
	constexpr Statement_Require(Id _id, shared_ptr<const Expression>&& _requiredProposition, shared_ptr<const Proof>&& _proof):
		id(_id), requiredProposition(std::move(_requiredProposition)), proof(std::move(_proof)) { }
	void print(unsigned int indentLevel) const override {
		printIndent(indentLevel);
		std::print("require {}:{} proves (", id.name, id.id);
		requiredProposition->print();
		std::print(") by (");
		proof->print();
		std::print(");");
	}
	~Statement_Require() override = default;
};


struct Statement_Atoms final : Statement {
	vector<Id> atomIds;
	constexpr Statement_Atoms(vector<Id>&& _atomIds):
		atomIds(std::move(_atomIds)) { }
	void print(unsigned int indentLevel) const override {
		printIndent(indentLevel);
		std::print("atom ");
		for (unsigned int i=0; i<atomIds.size(); i++) {
			if (i != 0) std::print(", ");
			std::print("{}", atomIds[i].name);
		}
		std::println(";");
	}
	~Statement_Atoms() override = default;
};


struct Statement_MustError final : Statement {
	shared_ptr<const Proof> proof;
	constexpr Statement_MustError(shared_ptr<const Proof>&& _proof):
		proof(std::move(_proof)) { }
	void print(unsigned int indentLevel) const override {
		printIndent(indentLevel);
		std::print("musterror ");
		proof->print();
		std::print(";");
	}
	~Statement_MustError() override = default;
};

}



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


void Proof_TryList::print() const {
	std::print("try (");
	for (unsigned int i=0; i<tryList.size(); i++) {
		if (i != 0) std::print(", ");
		tryList[i]->print();
	}
	std::print(")");
}
