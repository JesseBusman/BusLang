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
	vector<Id>& proofIdsAdded,
	vector<Id>& forAnyVarsIntroduced,
	vector<shared_ptr<const Expression>>& assumptionsIntroduced,
	bool printAssumptions
) {
	for (auto& s : statements) {
		if (auto print = dynamic_cast<const Statement_Print*>(s.get())) {
			auto provenProp = getProvenProp(proofId_to_provenProp, print->proof.get());
			
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
			auto forAnyVarsIntroducedBeforeBlock = forAnyVarsIntroduced.size();
			auto assumptionsIntroducedBeforeBlock = assumptionsIntroduced.size();
			
			runStatements(
				block->statements,
				proofId_to_provenProp,
				proofIdsAdded,
				forAnyVarsIntroduced,
				assumptionsIntroduced,
				printAssumptions
			);
			
			for (auto i=proofIdCountBeforeBlock; i<proofIdsAdded.size(); i++) proofId_to_provenProp.erase(proofIdsAdded[i]);
			proofIdsAdded.resize(proofIdCountBeforeBlock);
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
			if (printAssumptions) std::print("assuming {}:{} proves ", assumption->id.name, assumption->id.id); assumption->assumedProposition->print(); std::println();
		} else if (auto assumption = dynamic_cast<const Statement_SyntaxAssume*>(s.get())) {
			proofIdsAdded.push_back(assumption->id);
			proofId_to_provenProp[assumption->id] = assumption->assumedProposition;
			if (printAssumptions) std::print("assuming {}:{} proves ", assumption->id.name, assumption->id.id); assumption->assumedProposition->print(); std::println();
		} else if (auto requirement = dynamic_cast<const Statement_Require*>(s.get())) {
			const auto provenProp_ = getProvenProp(proofId_to_provenProp, requirement->proof.get());
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
						if (auto it2=matches_varVar_var_to_eqMaster.find(v2); it2 != matches_varVar_var_to_eqMaster.end()) {
							auto eqm2 = it2->second;
							if (eqm == eqm2) {
								std::print("require: "); reqProp->print(); std::println();
								std::print("proven: "); provenProp->print(); std::println();
								std::println("{}:{} and {}:{}  (matched {}:{})", v.name, v.id, v2.name, v2.id, eqm.name, eqm.id);
								throw ProofError("Vars matched that was supposed to remain independent"sv, requirement->proof->fileRange);
							}
						}
					}
				}
			}
			
			proofIdsAdded.push_back(requirement->id);
			proofId_to_provenProp[requirement->id] = requirement->requiredProposition;
			std::print("requiring {}:{} proves ", requirement->id.name, requirement->id.id); requirement->requiredProposition->print(); std::println();
		} else if (auto mustError = dynamic_cast<const Statement_MustError*>(s.get())) {
			try {
				auto provenProp = getProvenProp(proofId_to_provenProp, mustError->proof.get());
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
	const Proof* proof
) {
	//std::print("getProvenProp(");
	//proof->print();
	//std::println(")");
	
	if (auto block = dynamic_cast<const Proof_Block*>(proof)) {
		vector<Id> proofIdsAdded;
		vector<Id> forAnyVarsIntroduced;
		vector<shared_ptr<const Expression>> assumptionsIntroduced;
		runStatements(
			block->statements,
			proofId_to_provenProp,
			proofIdsAdded,
			forAnyVarsIntroduced,
			assumptionsIntroduced,
			false
		);
		auto ret = getProvenProp(proofId_to_provenProp, block->finalProof.get());
		for (auto& id : proofIdsAdded) proofId_to_provenProp.erase(id);
		
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
	} else if (auto substitution = dynamic_cast<const Proof_Substitute*>(proof)) {
		auto subProvenProp = getProvenProp(proofId_to_provenProp, substitution->subProof.get());
		
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
		auto leftProp  = getProvenProp(proofId_to_provenProp, rawShove->left .get());
		auto rightProp = getProvenProp(proofId_to_provenProp, rawShove->right.get());
		
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
		auto leftProp  = getProvenProp(proofId_to_provenProp, shove->left .get());
		auto rightProp = getProvenProp(proofId_to_provenProp, shove->right.get());
		
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
							std::print("left vars: "); for (auto& v : leftVars) std::print(" {}:{}", v.name, v.id);
							std::println();
							std::print("right vars: "); for (auto& v : rightVars) std::print(" {}:{}", v.name, v.id);
							std::println();
							std::print("left : "); leftProp ->print(); std::println();
							std::print("right: "); rightProp->print(); std::println();
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
								if (auto it2=matches_varVar_var_to_eqMaster.find(v2); it2 != matches_varVar_var_to_eqMaster.end()) {
									auto eqm2 = it2->second;
									if (eqm == eqm2) {
										std::println("{}:{} and {}:{}  (matched {}:{})", v.name, v.id, v2.name, v2.id, eqm.name, eqm.id);
										throw ProofError("Vars matched that was supposed to remain independent"sv, proof->fileRange);
									}
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
	} else if (auto tryList = dynamic_cast<const Proof_TryList*>(proof)) {
		vector<ProofError> proofErrors;
		for (auto& subProof : tryList->tryList) {
			try {
				return getProvenProp(proofId_to_provenProp, subProof.get());
			} catch (ProofError pe) {
				proofErrors.push_back(std::move(pe));
			}
		}
		std::println();
		for (const auto& pe : proofErrors) {
			std::println("- {}", pe.message);
		}
		std::println();
		throw ProofError("All proofs in try list failed"sv, tryList->fileRange);
	} else {
		std::print("\n");
		proof->print();
		std::print("\n");
		throw "Unimplemented proof expression type in getProvenProp";
	}
}
