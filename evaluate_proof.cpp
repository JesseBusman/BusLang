#include <memory>
#include <set>

#include "evaluate_proof.h"
#include "expression.h"
#include "proof.h"
#include "statement.h"
#include "id.h"
#include "globals.h"
#include "pattern_matching.h"

using std::string_view_literals::operator""sv;

void runStatements(
	const vector<shared_ptr<const Statement>>& statements,
	map<Id, shared_ptr<const Expression>>& proofId_to_provenProp,
	map<Id, pair<vector<Id>, shared_ptr<const Expression>>>& definitionId_to_varsAndExpression,
	vector<Id>& proofIdsAdded,
	vector<Id>& definitionIdsAdded,
	vector<Id>& forAnyVarsIntroduced,
	vector<shared_ptr<const Expression>>& assumptionsIntroduced
) {
	for (auto& s : statements) {
		if (auto print = dynamic_cast<const Statement_Print*>(s.get())) {
			auto provenProp = getProvenProp(proofId_to_provenProp, definitionId_to_varsAndExpression, print->proof.get());
			
			std::println();
			std::println();
			std::println();
			std::print("PRINT: proof  "); print->proof->print(); std::println();
			std::print("       proves "); provenProp->print(); std::println();
			std::println();
			std::println();
			std::println();
		} else if (auto block = dynamic_cast<const Statement_Block*>(s.get())) {
			auto proofIdCountBeforeBlock = proofIdsAdded.size();
			auto definitionIdCountBeforeBlock = definitionIdsAdded.size();
			auto forAnyVarsIntroducedBeforeBlock = forAnyVarsIntroduced.size();
			auto assumptionsIntroducedBeforeBlock = assumptionsIntroduced.size();
			
			runStatements(
				block->statements,
				proofId_to_provenProp,
				definitionId_to_varsAndExpression,
				proofIdsAdded,
				definitionIdsAdded,
				forAnyVarsIntroduced,
				assumptionsIntroduced
			);
			
			for (auto i=proofIdCountBeforeBlock; i<proofIdsAdded.size(); i++) proofId_to_provenProp.erase(proofIdsAdded[i]);
			proofIdsAdded.resize(proofIdCountBeforeBlock);
			for (auto i=definitionIdCountBeforeBlock; i<definitionIdsAdded.size(); i++) definitionId_to_varsAndExpression.erase(definitionIdsAdded[i]);
			definitionIdsAdded.resize(definitionIdCountBeforeBlock);
			forAnyVarsIntroduced.resize(forAnyVarsIntroducedBeforeBlock);
			assumptionsIntroduced.resize(assumptionsIntroducedBeforeBlock);
		} else if (dynamic_cast<const Statement_Atoms*>(s.get())) {
		} else if (auto forany = dynamic_cast<const Statement_ForAny*>(s.get())) {
			// TODO maybe error if var with same name is already introduced?
			forAnyVarsIntroduced.append_range(forany->varIds);
		} else if (auto assumption = dynamic_cast<const Statement_Assume*>(s.get())) {
			proofIdsAdded.push_back(assumption->id);
			proofId_to_provenProp[assumption->id] = assumption->assumedProposition;
			assumptionsIntroduced.push_back(assumption->assumedProposition);
		} else if (auto requirement = dynamic_cast<const Statement_Require*>(s.get())) {
			const auto provenProp_ = getProvenProp(proofId_to_provenProp, definitionId_to_varsAndExpression, requirement->proof.get());
			auto provenProp = provenProp_;
			
			auto reqProp = requirement->requiredProposition;
			
			vector<Id> provVars;
			provenProp = provenProp->unwrapForAnyVars(provVars);
			
			vector<Id> reqVars;
			reqProp = reqProp->unwrapForAnyVars(reqVars);
			
			std::set<Id> vars;
			for (auto& v : provVars) vars.insert(v);
			for (auto& v : reqVars) vars.insert(v);
			
			map<Id, std::set<Id>> matches_varVar_eqMaster_to_vars;
			map<Id, Id> matches_varVar_var_to_eqMaster;
			map<Id, shared_ptr<const Expression>> matches_var_to_expr;
			
			{
				auto mergeResult = mergeExprsWithVars(reqProp.get(), provenProp.get(), vars, matches_varVar_eqMaster_to_vars, matches_varVar_var_to_eqMaster, matches_var_to_expr);
				if (!mergeResult.has_value()) {
					std::print("\nIn requirement {}\nproof ", requirement->id.name);
					requirement->proof->print();
					std::print("\ndoes not prove    ");
					requirement->requiredProposition->print();
					std::print("\nbecause it proves ");
					provenProp_->print();
					std::println();
					std::println();
					std::print("Mismatch between: "); mergeResult.error().first ->print(); std::println();
					std::print("             and: "); mergeResult.error().second->print(); std::println();
					
					throw ProofError("Incorrect proof in require expression"sv, requirement->proof->fileRange, mergeResult.error().first->fileRange, mergeResult.error().second->fileRange);
				}
			}
			
			for (auto& v : reqVars) {
				if (auto it=matches_varVar_var_to_eqMaster.find(v); it != matches_varVar_var_to_eqMaster.end()) {
					auto eqm = it->second;
					
					if (matches_var_to_expr.contains(eqm)) {
						std::println("\nvar {}:{}", v.name, v.id);
						matches_var_to_expr[eqm]->print();
						std::println();
						throw ProofError("Var matched that was supposed to remain independent"sv, requirement->proof->fileRange);
					}
					for (auto& v2 : reqVars) {
						if (v == v2) continue;
						auto eqm2 = matches_varVar_var_to_eqMaster[v2];
						if (eqm == eqm2) {
							std::println("{}:{} and {}:{}", v.name, v.id, v2.name, v2.id);
							throw ProofError("Vars matched that was supposed to remain independent"sv, requirement->proof->fileRange);
						}
					}
				}
			}
			
			proofIdsAdded.push_back(requirement->id);
			proofId_to_provenProp[requirement->id] = requirement->requiredProposition;
			std::print("requiring {}:{} proves ", requirement->id.name, requirement->id.id); requirement->requiredProposition->print(); std::println();
		} else if (auto definition = dynamic_cast<const Statement_Define*>(s.get())) {
			if (definitionId_to_varsAndExpression.contains(definition->defId)) throw 9812498712;
			definitionId_to_varsAndExpression[definition->defId] = {definition->varIds, definition->rhs};
			definitionIdsAdded.push_back(definition->defId);
		} else if (auto mustError = dynamic_cast<const Statement_MustError*>(s.get())) {
			try {
				auto provenProp = getProvenProp(proofId_to_provenProp, definitionId_to_varsAndExpression, mustError->proof.get());
				std::println();
				std::println("musterror statement did not error:");
				std::print("proof  "); mustError->proof->print(); std::println();
				std::print("proves "); provenProp->print(); std::println();
				std::println();
				throw ProofError("musterror statement did not error!"sv, mustError->proof->fileRange);
			} catch (ProofError) {
			}
		} else {
			s->print(0);
			throw "Unimplemented statement type";
		}
	}
	
	
}


shared_ptr<const Expression> getProvenProp(
	map<Id, shared_ptr<const Expression>>& proofId_to_provenProp,
	map<Id, pair<vector<Id>, shared_ptr<const Expression>>>& definitionId_to_varsAndExpression,
	const Proof* proof
) {
	//std::print("getProvenProp(");
	//proof->print();
	//std::println(")");
	
	if (auto block = dynamic_cast<const Proof_Block*>(proof)) {
		vector<Id> proofIdsAdded;
		vector<Id> definitionIdsAdded;
		vector<Id> forAnyVarsIntroduced;
		vector<shared_ptr<const Expression>> assumptionsIntroduced;
		runStatements(
			block->statements,
			proofId_to_provenProp,
			definitionId_to_varsAndExpression,
			proofIdsAdded,
			definitionIdsAdded,
			forAnyVarsIntroduced,
			assumptionsIntroduced
		);
		auto ret = getProvenProp(proofId_to_provenProp, definitionId_to_varsAndExpression, block->finalProof.get());
		for (auto& id : proofIdsAdded) proofId_to_provenProp.erase(id);
		for (auto& id : definitionIdsAdded) definitionId_to_varsAndExpression.erase(id);
		
		for (auto it=assumptionsIntroduced.rbegin(); it != assumptionsIntroduced.rend(); it++) {
			ret = std::make_shared<Expression_Apply>(
				FileRange::span((*it)->fileRange, ret->fileRange),
				std::make_shared<Expression_Apply>(
					(*it)->fileRange,
					auto{ATOM_IMPLIES},
					std::move(*it)
				),
				std::move(ret)
			);
		}
		
		ret = wrapForAnyVars(std::move(forAnyVarsIntroduced), std::move(ret));
		return ret;
	} else if (auto _ = dynamic_cast<const Proof_RawWrap*>(proof)) {
		throw "TODO raw wrap";
	} else if (auto wrap = dynamic_cast<const Proof_Wrap*>(proof)) {
		auto subProvenProp = getProvenProp(proofId_to_provenProp, definitionId_to_varsAndExpression, wrap->subProof.get());
		auto it = definitionId_to_varsAndExpression.find(wrap->defId);
		if (it == definitionId_to_varsAndExpression.end()) {
			std::println("\nDefinition {}:{} does not exist\n", wrap->defId.name, wrap->defId.id);
			throw "Definition does not exist in wrap expression";
		}
		
		//std::println("WRAP:");
		
		//std::print("subProvenProp = "); subProvenProp->print(); std::println();
		//std::print("defExpr       = "); it->second.second->print(); std::println();
		
		vector<Id> subVars;
		vector<Id> defArgs;
		vector<Id> defForanyVars;
		
		auto subProvenProp_ = subProvenProp->unwrapForAnyVars(subVars);
		
		defArgs = it->second.first;
		
		auto defExpr = it->second.second->unwrapForAnyVars(defForanyVars);
		
		
		//std::print("subProvenProp_ = "); subProvenProp_->print(); std::println();
		//std::print("defExpr        = "); defExpr->print(); std::println();
		
		std::set<Id> vars;
		for (auto& v : subVars) vars.insert(v);
		for (auto& v : defArgs) vars.insert(v);
		for (auto& v : defForanyVars) vars.insert(v);
		
		map<Id, std::set<Id>> matches_varVar_eqMaster_to_vars;
		map<Id, Id> matches_varVar_var_to_eqMaster;
		map<Id, shared_ptr<const Expression>> matches_var_to_expr;
		
		{
			auto mergeResult = mergeExprsWithVars(defExpr.get(), subProvenProp_.get(), vars, matches_varVar_eqMaster_to_vars, matches_varVar_var_to_eqMaster, matches_var_to_expr);
			if (!mergeResult.has_value()) {
				defExpr->print();
				std::println();
				subProvenProp_->print();
				
				std::println();
				std::print("Mismatch between: "); mergeResult.error().first ->print(); std::println();
				std::print("             and: "); mergeResult.error().second->print(); std::println();
				
				throw ProofError("Wrap mismatch"sv, proof->fileRange, mergeResult.error().first->fileRange, mergeResult.error().second->fileRange);
			}
		}
		
		/*
		std::println("matches_varVar_eqMaster_to_vars:");
		for (auto& [eqm, vars] : matches_varVar_eqMaster_to_vars) {
			std::print("{}:{} :  ", eqm.name, eqm.id);
			for (auto& v : vars) std::print(" {}:{}", v.name, v.id);
			std::println();
		}
		std::println("matches_var_to_expr:");
		for (auto& [eqm, expr] : matches_var_to_expr) {
			std::print("{}:{} :  ", eqm.name, eqm.id);
			expr->print();
			std::println();
		}
		*/
		
		
		// forany vars inside a definition must match one-to-one with forany vars in the wrapped proposition
		// e.g.:
		// define Func x =    forany a,b: F a b x;
		// assume test proves forany u,p: F u p y;
		// wrap Func test     // <-- a matches u, b matches p. This is fine and gives a proof of   Func y
		// 
		// assume pest proves forany u:   F u u y;
		// wrap Func pest     // <-- ProofError
		// 
		// 
		// 
		// define Func2 x =   forany a,b,c: F (a b) c x
		// assume jest proves forany u,p:   F u p y
		// wrap Func2 jest    // This is fine, gives a proof of   Func2 y
		// 
		// 
		// 
		// define Func3 x =   forany a,b,c: F (a b) (b c) x
		// assume vest proves forany u,p:   F u p y
		// wrap Func3 vest    // This is fine, gives a proof of   Func3 y
		// 
		// assume zest proves forany u,p:   F u (u p) y
		// wrap Func3 zest    // <-- ProofError
		for (auto& v : defForanyVars) {
			if (auto it=matches_varVar_var_to_eqMaster.find(v); it != matches_varVar_var_to_eqMaster.end()) {
				auto eqm = it->second;
				if (matches_var_to_expr.contains(eqm)) {
					std::println("\nvar {}:{}", v.name, v.id);
					matches_var_to_expr[eqm]->print();
					std::println();
					throw ProofError("Var matched that was supposed to remain independent"sv, proof->fileRange);
				}
				for (auto& v2 : defForanyVars) {
					if (v == v2) continue;
					if (auto it2 = matches_varVar_var_to_eqMaster.find(v2); it2 != matches_varVar_var_to_eqMaster.end()) {
						auto eqm2 = it2->second;
						if (eqm == eqm2) {
							std::println("{}:{} and {}:{}", v.name, v.id, v2.name, v2.id);
							throw ProofError("Vars matched that was supposed to remain independent"sv, proof->fileRange);
						}
					}
				}
			}
			/*bool found = false;
			for (auto& v2 : subVars) {
				if (matches_varVar_eqMaster_to_vars[eqm].contains(v2)) { found = true; break; }
			}
			if (!found) {
				std::println("\n{}:{}", v.name, v.id);
				throw ProofError("Def forany var was not matched with any forany var"sv, proof->fileRange);
			}*/
		}
		
		
		
		/*
		define test x y = forany z: (P x) (Q y) (R z);
		assume test2 proves forany e,f: (P e) (Q A) (R f);
		wrap test test2
		
		var matches:
		e: e x
		f: f z
		
		expr matches
		y: A
		
		result:
		forany e: test e A
		
		
		
		
		
		define test3 x y = F x y;
		assume test4 proves forany a: a;
		print wrap test3 test4;
		// forany x,y: test3 x y
		
		assume test5 proves forany a,b: a b;
		print wrap test3 test5;
		// forany x,y: test3 x y
		
		*/
		
		
		
		
		vector<Id> defArgsWithoutDirectMatch;
		shared_ptr<const Expression> ret = std::make_shared<Expression_Id>(FileRange::none(), wrap->defId);
		for (auto& arg : defArgs) {
			std::shared_ptr<const Expression> substitution;
			if (auto it=matches_varVar_var_to_eqMaster.find(arg); it != matches_varVar_var_to_eqMaster.end()) {
				if (auto it2=matches_var_to_expr.find(it->second); it2 != matches_var_to_expr.end()) {
					substitution = it2->second;
				} else {
					substitution = std::make_shared<Expression_Id>(FileRange::none(), it->second);
				}
			} else {
				auto newId = Id::make(arg.name);
				substitution = std::make_shared<Expression_Id>(FileRange::none(), newId);
				defArgsWithoutDirectMatch.push_back(newId);
			}
			
			ret = std::make_shared<Expression_Apply>(
				proof->fileRange,
				std::move(ret),
				std::move(substitution)
			);
		}
		
		
		vector<Id> remainingSubVars;
		for (auto& v : subVars) {
			auto eqm = matches_varVar_var_to_eqMaster[v];
			if (eqm.id == 0) eqm = v;
			if (std::find(remainingSubVars.begin(), remainingSubVars.end(), eqm) != remainingSubVars.end()) continue;
			if (ret->containsId(eqm)) {
				remainingSubVars.push_back(eqm);
			}
		}
		
		remainingSubVars.append_range(defArgsWithoutDirectMatch);
		
		ret = wrapForAnyVars(std::move(remainingSubVars), std::move(ret));
				
		return ret;
	} else if (auto rawUnwrap = dynamic_cast<const Proof_RawUnwrap*>(proof)) {
		auto subProvenProp = getProvenProp(proofId_to_provenProp, definitionId_to_varsAndExpression, rawUnwrap->subProof.get());
		
		vector<shared_ptr<const Expression>> args;
		auto expr = subProvenProp;
		Id defId;
		while (true) {
			if (auto exprApply = dynamic_cast<const Expression_Apply*>(expr.get())) {
				args.push_back(exprApply->right);
				expr = exprApply->left;
			} else if (auto exprId = dynamic_cast<const Expression_Id*>(expr.get())) {
				defId = exprId->id;
				break;
			} else {
				std::println();
				subProvenProp->print();
				std::println();
				throw ProofError("Raw unwrap on proposition that isn't a definition invocation"sv, proof->fileRange);
			}
		}
		if (auto it=definitionId_to_varsAndExpression.find(defId); it != definitionId_to_varsAndExpression.end()) {
			if (it->second.first.size() != args.size()) {
				std::print("\nDefinition {}:{} has {} arguments, but unwrapped proposition has {}:\n", defId.name, defId.id, it->second.first.size(), args.size());
				subProvenProp->print();
				throw ProofError("Raw unwrap on definition invocation with incorrect amount of arguments"sv, proof->fileRange);
			}
			map<Id, shared_ptr<const Expression>> substitutions;
			for (unsigned int i=0; i<args.size(); i++) {
				substitutions[it->second.first[args.size()-1-i]] = std::move(args[i]);
			}
			
			return it->second.second->substitute(substitutions);
		} else {
			std::print("\n{}:{} is not a definition\n", defId.name, defId.id);
			throw ProofError("Raw unwrap on identifier that isn't a definition"sv, proof->fileRange);
		}
	} else if (auto unwrap = dynamic_cast<const Proof_Unwrap*>(proof)) {
		auto subProvenProp = getProvenProp(proofId_to_provenProp, definitionId_to_varsAndExpression, unwrap->subProof.get());
		
		vector<Id> forAnyArgs;
		subProvenProp = subProvenProp->unwrapForAnyVars(forAnyArgs);
		
		vector<shared_ptr<const Expression>> args;
		auto expr = subProvenProp;
		Id defId;
		while (true) {
			if (auto exprApply = dynamic_cast<const Expression_Apply*>(expr.get())) {
				args.push_back(exprApply->right);
				expr = exprApply->left;
			} else if (auto exprId = dynamic_cast<const Expression_Id*>(expr.get())) {
				defId = exprId->id;
				break;
			} else {
				std::println();
				subProvenProp->print();
				std::println();
				throw ProofError("Unwrap on proposition that isn't a definition invocation"sv, proof->fileRange);
			}
		}
		if (auto it=definitionId_to_varsAndExpression.find(defId); it != definitionId_to_varsAndExpression.end()) {
			if (it->second.first.size() != args.size()) {
				std::print("\nDefinition {}:{} has {} arguments, but unwrapped proposition has {}:\n", defId.name, defId.id, it->second.first.size(), args.size());
				subProvenProp->print();
				throw ProofError("Unwrap on definition invocation with incorrect amount of arguments"sv, proof->fileRange);
			}
			map<Id, shared_ptr<const Expression>> substitutions;
			for (unsigned int i=0; i<args.size(); i++) {
				substitutions[it->second.first[args.size()-1-i]] = std::move(args[i]);
			}
			
			shared_ptr<const Expression> ret = it->second.second->substitute(substitutions);
			if (forAnyArgs.size() != 0) {
				ret = wrapForAnyVars(std::move(forAnyArgs), std::move(ret));
			}
			return ret;
		} else {
			std::print("\n{}:{} is not a definition\n", defId.name, defId.id);
			throw ProofError("Unwrap on identifier that isn't a definition"sv, proof->fileRange);
		}
	} else if (auto substitution = dynamic_cast<const Proof_Substitute*>(proof)) {
		auto subProvenProp = getProvenProp(proofId_to_provenProp, definitionId_to_varsAndExpression, substitution->subProof.get());
		
		vector<Id> foranyVars;
		subProvenProp = subProvenProp->unwrapForAnyVars(foranyVars);
		
		if (foranyVars.size() == 0) throw ProofError("Substitution on non-forany proof"sv, proof->fileRange);
		map<Id, shared_ptr<const Expression>> substitutions;
		for (auto& [a, b] : substitution->vec) {
			bool found = false;
			for (auto& varId : foranyVars) {
				if (varId.name == a) {
					if (found) {
						std::println("\n{}", varId.name);
						throw ProofError("Substitution on var name that occurs multiple times"sv, proof->fileRange);
					}
					substitutions[varId] = b;
					found = true;
				}
			}
			if (!found) throw ProofError("substitution var does not exist in the forany"sv, proof->fileRange);
		}
		vector<Id> forAnyVarsNotSubstituted;
		forAnyVarsNotSubstituted.reserve(foranyVars.size() - substitution->vec.size());
		for (auto& varId : foranyVars) {
			if (substitutions.find(varId) == substitutions.end()) forAnyVarsNotSubstituted.push_back(varId);
		}
		auto newFAsubExpr = subProvenProp->substitute(substitutions);
		
		return wrapForAnyVars(std::move(forAnyVarsNotSubstituted), std::move(newFAsubExpr));
	} else if (auto id = dynamic_cast<const Proof_Id*>(proof)) {
		auto it = proofId_to_provenProp.find(id->id);
		if (it == proofId_to_provenProp.end()) {
			std::print("\nUnknown proof {}:{}\n", id->id.name, id->id.id);
			throw ProofError("Unknown proof"sv, proof->fileRange);
		}
		return it->second;
	} else if (auto rawShove = dynamic_cast<const Proof_RawShove*>(proof)) {
		auto leftProp  = getProvenProp(proofId_to_provenProp, definitionId_to_varsAndExpression, rawShove->left .get());
		auto rightProp = getProvenProp(proofId_to_provenProp, definitionId_to_varsAndExpression, rawShove->right.get());
		
		if (auto rightProp_ = dynamic_cast<const Expression_Apply*>(rightProp.get())) {
			if (auto rightPropLeft = dynamic_cast<const Expression_Apply*>(rightProp_->left.get())) {
				if (rightPropLeft->left->equals(ATOM_IMPLIES.get())) {
					if (leftProp->equals(rightPropLeft->right.get())) {
						return rightProp_->right;
					} else {
						std::println();
						leftProp->print();
						std::println();
						rightPropLeft->right->print();
						std::println();
						throw ProofError("Raw shove mismatch"sv, rawShove->fileRange);
					}
				}
			}
		}
		std::println();
		rightProp->print();
		std::println();
		throw ProofError("Right hand side of raw shove is not an implication!"sv, rawShove->fileRange);
	} else if (auto shove = dynamic_cast<const Proof_Shove*>(proof)) {
		auto leftProp  = getProvenProp(proofId_to_provenProp, definitionId_to_varsAndExpression, shove->left .get());
		auto rightProp = getProvenProp(proofId_to_provenProp, definitionId_to_varsAndExpression, shove->right.get());
		
		//std::print("leftProp  = "); leftProp ->print(); std::println();
		//std::print("rightProp = "); rightProp->print(); std::println();
		
		vector<Id> leftVars;
		vector<Id> rightVars;
		leftProp  = leftProp ->unwrapForAnyVars(leftVars );
		rightProp = rightProp->unwrapForAnyVars(rightVars);
		
		if (leftVars.size() != 0) {
			map<Id, shared_ptr<const Expression>> substitutions;
			for (auto& varId : leftVars) {
				auto newId = Id::make(varId.name);
				substitutions[varId] = std::make_shared<const Expression_Id>(FileRange::none(), newId);
				varId = newId;
			}
			leftProp = leftProp->substitute(substitutions);
		}
		if (rightVars.size() != 0) {
			map<Id, shared_ptr<const Expression>> substitutions;
			for (auto& varId : rightVars) {
				auto newId = Id::make(varId.name);
				substitutions[varId] = std::make_shared<const Expression_Id>(FileRange::none(), newId);
				varId = newId;
			}
			rightProp = rightProp->substitute(substitutions);
		}
		
		//std::print("leftProp  = "); leftProp ->print(); std::println();
		//std::print("rightProp = "); rightProp->print(); std::println();
		
		if (auto rightProp_ = dynamic_cast<const Expression_Apply*>(rightProp.get())) {
			if (auto rightPropLeft = dynamic_cast<const Expression_Apply*>(rightProp_->left.get())) {
				if (auto implId = dynamic_cast<const Expression_Id*>(rightPropLeft->left.get()); implId && implId->id == ATOM_IMPLIES->id) {
					vector<Id> mustRemainIndependentVarsVars;
					
					auto rightPropLeftRight__ = rightPropLeft->right->unwrapForAnyVars(mustRemainIndependentVarsVars);
					
					map<Id, std::set<Id>> matches_varVar_eqMaster_to_vars;
					map<Id, Id> matches_varVar_var_to_eqMaster;
					map<Id, shared_ptr<const Expression>> matches_var_to_expr;
					std::set<Id> allVars;
					for (auto& v :  leftVars) allVars.insert(v);
					for (auto& v : rightVars) allVars.insert(v);
					for (auto& v : mustRemainIndependentVarsVars) allVars.insert(v);
					//std::print("allVars = [");
					//for (auto v : allVars) std::print("{}:{}, ", v.name, v.id);
					//std::println("];");
					{
						auto mergeResult = mergeExprsWithVars(leftProp.get(), rightPropLeftRight__.get(), allVars, matches_varVar_eqMaster_to_vars, matches_varVar_var_to_eqMaster, matches_var_to_expr);
						if (!mergeResult.has_value()) {
							std::println();
							leftProp->print();
							std::println();
							rightProp->print();
							std::println();
							std::println();
							std::print("Mismatch between: "); mergeResult.error().first ->print(); std::println();
							std::print("             and: "); mergeResult.error().second->print(); std::println();
							
							throw ProofError("Shove mismatch"sv, proof->fileRange, mergeResult.error().first->fileRange, mergeResult.error().second->fileRange);
						}
					}
					
					for (auto& v : mustRemainIndependentVarsVars) {
						if (auto it=matches_varVar_var_to_eqMaster.find(v); it != matches_varVar_var_to_eqMaster.end()) {
							auto eqm = it->second;
							if (matches_var_to_expr.contains(eqm)) {
								std::println("\nvar {}:{}", v.name, v.id);
								matches_var_to_expr[eqm]->print();
								std::println();
								throw ProofError("Var matched that was supposed to remain independent"sv, proof->fileRange);
							}
							for (auto& v2 : mustRemainIndependentVarsVars) {
								if (v == v2) continue;
								auto eqm2 = matches_varVar_var_to_eqMaster[v2];
								if (eqm == eqm2) {
									std::println("{}:{} and {}:{}", v.name, v.id, v2.name, v2.id);
									throw ProofError("Vars matched that was supposed to remain independent"sv, proof->fileRange);
								}
							}
						}
					}
					
					map<Id, shared_ptr<const Expression>> substitutions;
					for (auto& [from, to] : matches_varVar_var_to_eqMaster) {
						substitutions[from] = std::make_shared<Expression_Id>(FileRange::none(), to);
					}
					for (auto& [from, to] : matches_var_to_expr) {
						for (auto& from2 : matches_varVar_eqMaster_to_vars[from]) {
							substitutions[from2] = to;
						}
					}
					
					auto retWithSubstitutions = rightProp_->right->substitute(substitutions);
					
					vector<Id> retVars;
					for (auto& var : allVars) {
						if (retWithSubstitutions->containsId(var)) retVars.push_back(var);
					}
					
					return wrapForAnyVars(std::move(retVars), std::move(retWithSubstitutions));
				}
			}
		}
		
		std::println();
		rightProp->print();
		std::println();
		throw ProofError("Right hand side of shove is not an implication!"sv, shove->right->fileRange);
	} else {
		std::print("\n");
		proof->print();
		std::print("\n");
		throw "Unimplemented proof expression type in getProvenProp";
	}
}
