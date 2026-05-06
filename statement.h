#pragma once

#include <vector>
#include <memory>
#include <print>

#include "id.h"

using std::shared_ptr;
using std::pair;
using std::vector;

struct Proof;
struct Expression;


struct Statement {
	virtual void print(unsigned int indentLevel) const = 0;
	virtual ~Statement() = default;
};


struct Statement_Block final : Statement {
	vector<shared_ptr<const Statement>> statements;
	constexpr Statement_Block(vector<shared_ptr<const Statement>>&& _statements):
		statements(std::move(_statements)) { }
	void print(unsigned int indentLevel) const override;
	~Statement_Block() override = default;
};


struct Statement_Print final : Statement {
	shared_ptr<const Proof> proof;
	constexpr Statement_Print(shared_ptr<const Proof>&& _proof):
		proof(std::move(_proof)) { }
	void print(unsigned int indentLevel) const override;
	~Statement_Print() override = default;
};


struct Statement_ForAny final : Statement {
	vector<Id> varIds; 
	constexpr Statement_ForAny(vector<Id>&& _varIds):
		varIds(std::move(_varIds)) { }
	void print(unsigned int indentLevel) const override;
	~Statement_ForAny() override = default;
};


struct Statement_Assume final : Statement {
	Id id;
	shared_ptr<const Expression> assumedProposition;
	constexpr Statement_Assume(Id _id, shared_ptr<const Expression>&& _assumedProposition):
		id(_id), assumedProposition(std::move(_assumedProposition)) { }
	void print(unsigned int indentLevel) const override;
	~Statement_Assume() override = default;
};


struct Statement_SyntaxAssume final : Statement {
	Id id;
	shared_ptr<const Expression> assumedProposition;
	constexpr Statement_SyntaxAssume(Id _id, shared_ptr<const Expression>&& _assumedProposition):
		id(_id), assumedProposition(std::move(_assumedProposition)) { }
	void print(unsigned int indentLevel) const override;
	~Statement_SyntaxAssume() override = default;
};


struct Statement_Require final : Statement {
	Id id;
	shared_ptr<const Expression> requiredProposition;
	shared_ptr<const Proof> proof;
	constexpr Statement_Require(Id _id, shared_ptr<const Expression>&& _requiredProposition, shared_ptr<const Proof>&& _proof):
		id(_id), requiredProposition(std::move(_requiredProposition)), proof(std::move(_proof)) { }
	void print(unsigned int indentLevel) const override;
	~Statement_Require() override = default;
};


struct Statement_Atoms final : Statement {
	vector<Id> atomIds;
	constexpr Statement_Atoms(vector<Id>&& _atomIds):
		atomIds(std::move(_atomIds)) { }
	void print(unsigned int indentLevel) const override;
	~Statement_Atoms() override = default;
};


struct Statement_MustError final : Statement {
	shared_ptr<const Proof> proof;
	constexpr Statement_MustError(shared_ptr<const Proof>&& _proof):
		proof(std::move(_proof)) { }
	void print(unsigned int indentLevel) const override;
	~Statement_MustError() override = default;
};

