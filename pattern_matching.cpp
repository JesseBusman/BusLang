#include <utility>
#include <memory>
#include <set>
#include <map>
#include <expected>

#include "pattern_matching.h"
#include "expression.h"

using std::pair;
using std::shared_ptr;
using std::map;

[[nodiscard]] std::expected<void*, pair<shared_ptr<const Expression>, shared_ptr<const Expression>>> mergeExprsWithVars(
	const Expression* a,
	const Expression* b,
	const std::set<Id>& vars,
	map<Id, std::set<Id>>& matches_varVar_eqMaster_to_vars,
	map<Id, Id>& matches_varVar_var_to_eqMaster,
	map<Id, shared_ptr<const Expression>>& matches_var_to_expr
);

[[nodiscard]] static std::expected<void*, pair<shared_ptr<const Expression>, shared_ptr<const Expression>>> addVarVarMatch(
	Id aId,
	Id bId,
	const std::set<Id>& vars,
	map<Id, std::set<Id>>& matches_varVar_eqMaster_to_vars,
	map<Id, Id>& matches_varVar_var_to_eqMaster,
	map<Id, shared_ptr<const Expression>>& matches_var_to_expr
) {
	auto getEqMaster = [&](Id varId) -> Id{
		if (auto it=matches_varVar_var_to_eqMaster.find(varId); it != matches_varVar_var_to_eqMaster.end()) {
			return it->second;
		} else {
			matches_varVar_eqMaster_to_vars[varId] = {varId};
			matches_varVar_var_to_eqMaster[varId] = varId;
			return varId;
		}
	};
	
	auto aEqm = getEqMaster(aId);
	auto bEqm = getEqMaster(bId);
	
	// Shortcut if they were already matched
	if (aEqm == bEqm) return nullptr;
	
	// Substitute b with a in all expressions
	{
		map<Id, shared_ptr<const Expression>> substitutions;
		substitutions[bEqm] = std::make_shared<Expression_Id>(FileRange::none(), aEqm);
		for (auto it=matches_var_to_expr.begin(); it != matches_var_to_expr.end(); it++) {
			it->second = it->second->substitute(substitutions);
		}
	}
	
	// Merge var-var matches
	for (auto& varId : matches_varVar_eqMaster_to_vars[bEqm]) {
		matches_varVar_var_to_eqMaster[varId] = aEqm;
	}
	matches_varVar_eqMaster_to_vars[aEqm].merge(matches_varVar_eqMaster_to_vars[bEqm]);
	matches_varVar_eqMaster_to_vars.erase(bEqm);
	
	// Merge expr matches
	if (auto itA=matches_var_to_expr.find(aEqm); itA != matches_var_to_expr.end()) {
		if (auto itB=matches_var_to_expr.find(bEqm); itB != matches_var_to_expr.end()) {
			return mergeExprsWithVars(itA->second.get(), itB->second.get(), vars, matches_varVar_eqMaster_to_vars, matches_varVar_var_to_eqMaster, matches_var_to_expr);
		} else {
			return nullptr;
		}
	} else if (auto itB=matches_var_to_expr.find(bEqm); itB != matches_var_to_expr.end()) {
		matches_var_to_expr[aEqm] = std::move(itB->second);
		matches_var_to_expr.erase(bEqm);
		return nullptr;
	} else {
		return nullptr;
	}
}

[[nodiscard]] static std::expected<void*, pair<shared_ptr<const Expression>, shared_ptr<const Expression>>> addVarExprMatch(
	Id varId,
	const Expression* expr,
	const std::set<Id>& vars,
	map<Id, std::set<Id>>& matches_varVar_eqMaster_to_vars,
	map<Id, Id>& matches_varVar_var_to_eqMaster,
	map<Id, shared_ptr<const Expression>>& matches_var_to_expr
) {
	auto getEqMaster = [&](Id varId) -> Id{
		if (auto it=matches_varVar_var_to_eqMaster.find(varId); it != matches_varVar_var_to_eqMaster.end()) {
			return it->second;
		} else {
			matches_varVar_eqMaster_to_vars[varId] = {varId};
			matches_varVar_var_to_eqMaster[varId] = varId;
			return varId;
		}
	};
	
	Id eqm = getEqMaster(varId);
	
	shared_ptr<const Expression> expr_;
	{
		map<Id, shared_ptr<const Expression>> substitutions;
		for (auto& [from, to] : matches_varVar_var_to_eqMaster) substitutions[from] = std::make_shared<Expression_Id>(FileRange::none(), to);
		for (auto& [from, to] : matches_var_to_expr) {
			for (auto& from2 : matches_varVar_eqMaster_to_vars[from]) substitutions[from2] = to;
		}
		expr_ = expr->substitute(substitutions);
	}
	
	if (expr_->containsId(eqm)) {
		std::println("Variable {}:{} matched with pattern that contains itself:", eqm.name, eqm.id);
		expr_->print(); std::println();
		return std::unexpected(pair{std::make_shared<Expression_Id>(FileRange::none(), eqm), expr->shared_from_this()});
	}
	
	if (auto it=matches_var_to_expr.find(eqm); it != matches_var_to_expr.end()) {
		auto other = it->second;
		if (other->containsId(eqm)) {
			std::println("Variable {}:{} matched with pattern that contains itself:", eqm.name, eqm.id);
			other->print(); std::println();
			return std::unexpected(pair{std::make_shared<Expression_Id>(FileRange::none(), eqm), other});
		}
		return mergeExprsWithVars(
			expr_.get(),
			other.get(),
			vars, matches_varVar_eqMaster_to_vars, matches_varVar_var_to_eqMaster, matches_var_to_expr
		);
	} else {
		map<Id, shared_ptr<const Expression>> substitutions;
		substitutions[eqm] = expr_;
		for (auto it=matches_var_to_expr.begin(); it != matches_var_to_expr.end(); it++) {
			it->second = it->second->substitute(substitutions);
			if (it->second->containsId(it->first)) {
				std::println("Variable {}:{} matched with pattern that contains itself:", it->first.name, it->first.id);
				it->second->print(); std::println();
				return std::unexpected(pair{std::make_shared<Expression_Id>(FileRange::none(), it->first), it->second});
			}
		}
		
		matches_var_to_expr[eqm] = expr_;
		return nullptr;
	}
}


[[nodiscard]] std::expected<void*, pair<shared_ptr<const Expression>, shared_ptr<const Expression>>> mergeExprsWithVars(
	const Expression* a,
	const Expression* b,
	const std::set<Id>& vars,
	map<Id, std::set<Id>>& matches_varVar_eqMaster_to_vars,
	map<Id, Id>& matches_varVar_var_to_eqMaster,
	map<Id, shared_ptr<const Expression>>& matches_var_to_expr
) {
	//std::println("mergeExprsWithVars on:");
	//std::print("   "); a->print(); std::println();
	//std::print("   "); b->print(); std::println();
	
	if (auto aId = dynamic_cast<const Expression_Id*>(a); aId && vars.contains(aId->id)) {
		if (auto bId = dynamic_cast<const Expression_Id*>(b); bId && vars.contains(bId->id)) {
			return addVarVarMatch(aId->id, bId->id, vars, matches_varVar_eqMaster_to_vars, matches_varVar_var_to_eqMaster, matches_var_to_expr);
		} else {
			return addVarExprMatch(aId->id, b, vars, matches_varVar_eqMaster_to_vars, matches_varVar_var_to_eqMaster, matches_var_to_expr);
		}
	} else {
		if (auto bId = dynamic_cast<const Expression_Id*>(b); bId && vars.contains(bId->id)) {
			return addVarExprMatch(bId->id, a, vars, matches_varVar_eqMaster_to_vars, matches_varVar_var_to_eqMaster, matches_var_to_expr);
		}
	}
	
	
	
	if (auto aId = dynamic_cast<const Expression_Id*>(a)) {
		if (auto bId = dynamic_cast<const Expression_Id*>(b)) {
			if (aId->id == bId->id) {
				return nullptr;
			} else {
				goto mismatched;
			}
		} else {
			goto mismatched;
		}
	} else if (dynamic_cast<const Expression_Id*>(b)) {
		goto mismatched;
	}
	
	
	
	if (auto aFA = dynamic_cast<const Expression_ForAny*>(a)) {
		if (auto bFA = dynamic_cast<const Expression_ForAny*>(b)) {
			if (aFA->varIds.size() != bFA->varIds.size()) goto mismatched;
			
			map<Id, shared_ptr<const Expression>> substitutions;
			for (unsigned int i=0; i<aFA->varIds.size(); i++) {
				substitutions[bFA->varIds[i]] = std::make_shared<Expression_Id>(FileRange::none(), aFA->varIds[i]);
			}
			
			return mergeExprsWithVars(aFA->subExpr.get(), bFA->subExpr->substitute(substitutions).get(), vars, matches_varVar_eqMaster_to_vars, matches_varVar_var_to_eqMaster, matches_var_to_expr);
		} else {
			goto mismatched;
		}
	} else if (dynamic_cast<const Expression_ForAny*>(b)) {
		goto mismatched;
	}
	
	
	
	if (auto aApply = dynamic_cast<const Expression_Apply*>(a)) {
		if (auto bApply = dynamic_cast<const Expression_Apply*>(b)) {
			auto leftResult = mergeExprsWithVars(aApply->left .get(), bApply->left .get(), vars, matches_varVar_eqMaster_to_vars, matches_varVar_var_to_eqMaster, matches_var_to_expr);
			if (!leftResult.has_value()) return std::unexpected(std::move(leftResult.error()));
			return mergeExprsWithVars(aApply->right.get(), bApply->right.get(), vars, matches_varVar_eqMaster_to_vars, matches_varVar_var_to_eqMaster, matches_var_to_expr);
		} else {
			goto mismatched;
		}
	} else if (dynamic_cast<const Expression_Apply*>(b)) {
		goto mismatched;
	}
	
	
	std::println();
	a->print();
	std::println();
	b->print();
	std::println();
	
	
	throw "Unimplemented expr types in mergeExprsWithVars";
	
	mismatched:
	return std::unexpected(pair{a->shared_from_this(), b->shared_from_this()});
}
