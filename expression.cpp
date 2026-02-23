#include <memory>
#include <map>
#include <print>
#include <vector>

#include "id.h"
#include "file_range.h"
#include "globals.h"
#include "expression.h"

using std::shared_ptr;
using std::map;
using std::vector;







[[nodiscard]] bool Expression_Id::patternMatch(const Expression* other, map<Id, shared_ptr<const Expression>>& matches) const {
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
shared_ptr<const Expression> Expression_Id::substitute(const map<Id, shared_ptr<const Expression>>& substitutions) const {
	auto it = substitutions.find(id);
	if (it != substitutions.end()) return it->second;
	else return std::dynamic_pointer_cast<const Expression>(shared_from_this());
}







[[nodiscard]] bool Expression_Apply::patternMatch(const Expression* other, map<Id, shared_ptr<const Expression>>& matches) const {
	if (auto other_ = dynamic_cast<const Expression_Apply*>(other)) {
		return left->patternMatch(other_->left.get(), matches) && right->patternMatch(other_->right.get(), matches);
	} else {
		return false;
	}
}
[[nodiscard]] shared_ptr<const Expression> Expression_Apply::unwrapForAnyVars(vector<Id>& out) const {
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
[[nodiscard]] bool Expression_Apply::containsId(Id id) const {
	return left->containsId(id) || right->containsId(id);
}
void Expression_Apply::print(int maxPrecedence) const {
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
shared_ptr<const Expression> Expression_Apply::substitute(const map<Id, shared_ptr<const Expression>>& substitutions) const {
	auto newLeft  = left ->substitute(substitutions);
	auto newRight = right->substitute(substitutions);
	
	if (newLeft == left && newRight == right) return std::dynamic_pointer_cast<const Expression>(shared_from_this());
	else return std::make_shared<Expression_Apply>(fileRange, std::move(newLeft), std::move(newRight));
}







[[nodiscard]] bool Expression_ForAny::patternMatch(const Expression* other, map<Id, shared_ptr<const Expression>>& matches) const {
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
[[nodiscard]] shared_ptr<const Expression> Expression_ForAny::unwrapForAnyVars(vector<Id>& out) const {
	out.append_range(varIds);
	return subExpr->unwrapForAnyVars(out);
}
void Expression_ForAny::print(int maxPrecedence) const {
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
shared_ptr<const Expression> Expression_ForAny::substitute(const map<Id, shared_ptr<const Expression>>& substitutions) const {
	auto newSubExpr = subExpr->substitute(substitutions);
	if (newSubExpr == subExpr) return std::dynamic_pointer_cast<const Expression>(shared_from_this());
	else return std::make_shared<Expression_ForAny>(fileRange, auto{varIds}, std::move(newSubExpr));
}


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
