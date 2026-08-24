export module BusLang:Expression;

import std;
import :Id;
import :FileRange;

using std::shared_ptr;
using std::map;
using std::vector;



export {


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
	[[nodiscard]] bool patternMatch(const Expression* other, map<Id, shared_ptr<const Expression>>& matches) const override {
		if (auto it=matches.find(id); it != matches.end()) {
			if (it->second == nullptr) {
				it->second = other->shared_from_this();
				return true;
			} else {
				return it->second->equals(other);
			}
		} else if (auto other_ = dynamic_cast<const Expression_Id*>(other)) {
			return id == other_->id;
		} else {
			return false;
		}
	}
	shared_ptr<const Expression> substitute(const map<Id, shared_ptr<const Expression>>& substitutions) const override {
		auto it = substitutions.find(id);
		if (it != substitutions.end()) return it->second;
		else return std::dynamic_pointer_cast<const Expression>(shared_from_this());
	}
	inline ~Expression_Id() override = default;
};



std::shared_ptr<const Expression_Id> ATOM_IMPLIES;



struct Expression_Apply final : Expression {
	shared_ptr<const Expression> left;
	shared_ptr<const Expression> right;
	constexpr Expression_Apply(FileRange _fileRange, shared_ptr<const Expression>&& _left, shared_ptr<const Expression>&& _right):
		Expression(_fileRange), left(std::move(_left)), right(std::move(_right))
	{
	}
	[[nodiscard]] bool patternMatch(const Expression* other, map<Id, shared_ptr<const Expression>>& matches) const override {
		if (auto other_ = dynamic_cast<const Expression_Apply*>(other)) {
			return left->patternMatch(other_->left.get(), matches) && right->patternMatch(other_->right.get(), matches);
		} else {
			return false;
		}
	}
	[[nodiscard]] shared_ptr<const Expression> unwrapForAnyVars(vector<Id>& out) const override {
		if (auto left_ = dynamic_cast<const Expression_Apply*>(left.get())) {
			if (auto implId = dynamic_cast<const Expression_Id*>(left_->left.get()); implId && implId->id == ATOM_IMPLIES->id) {
				auto newRight = right->unwrapForAnyVars(out);
				if (newRight != right) {
					return std::make_shared<Expression_Apply>(fileRange, auto{left}, std::move(newRight));
				}
			}
		}
		return std::dynamic_pointer_cast<const Expression>(shared_from_this());
	}
	[[nodiscard]] bool containsId(Id id) const override {
		return left->containsId(id) || right->containsId(id);
	}
	void print(int maxPrecedence) const override {
		if (auto left_ = dynamic_cast<const Expression_Apply*>(left.get())) {
			if (auto implId = dynamic_cast<const Expression_Id*>(left_->left.get()); implId && implId->id == ATOM_IMPLIES->id) {
				if (maxPrecedence < 5) std::print("( ");
				left_->right->print(4);
				std::print(" -> ");
				right->print(5);
				if (maxPrecedence < 5) std::print(" )");
				return;
			}
		}
		
		if (maxPrecedence < 0) std::print("( ");
		left->print(0);
		std::print(" ");
		right->print(-1);
		if (maxPrecedence < 0) std::print(" )");
	}
	shared_ptr<const Expression> substitute(const map<Id, shared_ptr<const Expression>>& substitutions) const override {
		auto newLeft  = left ->substitute(substitutions);
		auto newRight = right->substitute(substitutions);
		
		if (newLeft == left && newRight == right) return std::dynamic_pointer_cast<const Expression>(shared_from_this());
		else return std::make_shared<Expression_Apply>(fileRange, std::move(newLeft), std::move(newRight));
	}
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
	[[nodiscard]] bool patternMatch(const Expression* other, map<Id, shared_ptr<const Expression>>& matches) const override {
		if (auto other_ = dynamic_cast<const Expression_ForAny*>(other)) {
			if (other_->varIds.size() != varIds.size()) return false;
			
			map<Id, shared_ptr<const Expression>> substitutions;
			for (unsigned int i=0; i<varIds.size(); i++) {
				substitutions[other_->varIds[i]] = std::make_shared<Expression_Id>(FileRange::none(), varIds[i]);
			}
			
			return subExpr->patternMatch(other_->subExpr->substitute(substitutions).get(), matches);
		} else {
			return false;
		}
	}
	[[nodiscard]] shared_ptr<const Expression> unwrapForAnyVars(vector<Id>& out) const override {
		out.append_range(varIds);
		return subExpr->unwrapForAnyVars(out);
	}
	void print(int maxPrecedence) const override {
		if (maxPrecedence <= 10) std::print("( ");
		std::print("forany ");
		for (unsigned int i=0; i<varIds.size(); i++) {
			if (i != 0) std::print(", ");
			std::print("{}:{}", varIds[i].name, varIds[i].id);
		}
		std::print(": ");
		subExpr->print(20);
		if (maxPrecedence <= 10) std::print(" )");
	}
	shared_ptr<const Expression> substitute(const map<Id, shared_ptr<const Expression>>& substitutions) const override {
		auto newSubExpr = subExpr->substitute(substitutions);
		if (newSubExpr == subExpr) return std::dynamic_pointer_cast<const Expression>(shared_from_this());
		else return std::make_shared<Expression_ForAny>(fileRange, auto{varIds}, std::move(newSubExpr));
	}
	inline ~Expression_ForAny() override = default;
};



/*
struct Expression_WithLazySubstitution final : Expression {
	
	~Expression_WithLazySubstitution() override = default;
};
*/


shared_ptr<const Expression> wrapForAnyVars(vector<Id>&& forAnyVars, shared_ptr<const Expression>&& expr) {
	if (forAnyVars.size() == 0) {
		return std::move(expr);
	} else if (auto fa = dynamic_cast<const Expression_ForAny*>(expr.get())) {
		forAnyVars.append_range(fa->varIds);
		return std::make_shared<Expression_ForAny>(expr->fileRange, std::move(forAnyVars), auto{fa->subExpr});
	} else {
		return std::make_shared<Expression_ForAny>(expr->fileRange, std::move(forAnyVars), std::move(expr));
	}
}

}
