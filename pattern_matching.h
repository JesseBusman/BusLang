#pragma once

#include <expected>
#include <map>
#include <memory>
#include <utility>
#include <set>

struct Expression;
struct Id;

using std::pair;
using std::map;
using std::shared_ptr;

[[nodiscard]] std::expected<void*, pair<shared_ptr<const Expression>, shared_ptr<const Expression>>> mergeExprsWithVars(
	const Expression* a,
	const Expression* b,
	const std::set<Id>& vars,
	map<Id, std::set<Id>>& matches_varVar_eqMaster_to_vars,
	map<Id, Id>& matches_varVar_var_to_eqMaster,
	map<Id, shared_ptr<const Expression>>& matches_var_to_expr
);
