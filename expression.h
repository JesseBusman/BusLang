#pragma once

#include <memory>
#include <map>
#include <print>
#include <vector>

#include "id.h"
#include "file_range.h"

using std::shared_ptr;
using std::map;
using std::vector;


struct Expression : std::enable_shared_from_this<Expression> {
	FileRange fileRange;
protected:
	constexpr Expression(FileRange _fileRange):
		fileRange(_fileRange) { }
public:
	virtual void print(int maxPrecedence=500) const = 0;
	[[nodiscard]] virtual shared_ptr<const Expression> substitute(const map<Id, shared_ptr<const Expression>>& substitutions) const = 0;
	[[nodiscard]] virtual bool patternMatch(const Expression* other /* not the pattern */, map<Id, shared_ptr<const Expression>>& matches) const = 0;
	[[nodiscard]] virtual bool containsId(Id id) const = 0;
	[[nodiscard]] virtual shared_ptr<const Expression> unwrapForAnyVars(vector<Id>& out) const = 0;
	[[nodiscard]] inline bool equals(const Expression* other) const {
		map<Id, shared_ptr<const Expression>> dummy;
		return this->patternMatch(other, dummy);
	}
	virtual inline ~Expression() = default;
};


struct Expression_Id final : Expression {
	Id id;
	constexpr Expression_Id(FileRange _fileRange, Id _id):
		Expression(_fileRange), id(_id) { }
	inline void print(int maxPrecedence) const override {
		std::print("{}:{}", id.name, id.id);
	}
	[[nodiscard]] constexpr bool containsId(Id _id) const override {
		return id == _id;
	}
	[[nodiscard]] constexpr shared_ptr<const Expression> unwrapForAnyVars(vector<Id>& out) const override {
		return std::dynamic_pointer_cast<const Expression>(shared_from_this());
	}
	[[nodiscard]] bool patternMatch(const Expression* other, map<Id, shared_ptr<const Expression>>& matches) const override;
	shared_ptr<const Expression> substitute(const map<Id, shared_ptr<const Expression>>& substitutions) const override;
};


struct Expression_Apply final : Expression {
	shared_ptr<const Expression> left;
	shared_ptr<const Expression> right;
	constexpr Expression_Apply(FileRange _fileRange, shared_ptr<const Expression>&& _left, shared_ptr<const Expression>&& _right):
		Expression(_fileRange), left(std::move(_left)), right(std::move(_right))
	{
	}
	[[nodiscard]] bool patternMatch(const Expression* other, map<Id, shared_ptr<const Expression>>& matches) const override;
	[[nodiscard]] shared_ptr<const Expression> unwrapForAnyVars(vector<Id>& out) const override;
	[[nodiscard]] bool containsId(Id id) const override;
	void print(int maxPrecedence) const override;
	shared_ptr<const Expression> substitute(const map<Id, shared_ptr<const Expression>>& substitutions) const override;
	inline ~Expression_Apply() override = default;
};


struct Expression_ForAny final : Expression {
	vector<Id> varIds; 
	shared_ptr<const Expression> subExpr;
	[[nodiscard]] bool containsId(Id id) const override {
		return subExpr->containsId(id);
	}
	constexpr Expression_ForAny(FileRange _fileRange, vector<Id>&& _varIds, shared_ptr<const Expression>&& _subExpr):
		Expression(_fileRange), varIds(std::move(_varIds)), subExpr(std::move(_subExpr)) { }
	[[nodiscard]] bool patternMatch(const Expression* other, map<Id, shared_ptr<const Expression>>& matches) const override;
	[[nodiscard]] shared_ptr<const Expression> unwrapForAnyVars(vector<Id>& out) const override;
	void print(int maxPrecedence) const override;
	shared_ptr<const Expression> substitute(const map<Id, shared_ptr<const Expression>>& substitutions) const override;
	inline ~Expression_ForAny() override = default;
};


/*
struct Expression_WithLazySubstitution final : Expression {
	
	~Expression_WithLazySubstitution() override = default;
};
*/


shared_ptr<const Expression> wrapForAnyVars(vector<Id>&& forAnyVars, shared_ptr<const Expression>&& expr);
