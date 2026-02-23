#pragma once

#include <memory>
#include <map>
#include <vector>

#include "file_range.h"

struct Id;
struct Expression;
struct Proof;
struct Statement;


std::shared_ptr<const Expression> getProvenProp(
	std::map<Id, std::shared_ptr<const Expression>>& proofId_to_provenProp,
	std::map<Id, std::pair<std::vector<Id>, std::shared_ptr<const Expression>>>& definitionId_to_varsAndExpression,
	const Proof* proof
);

void runStatements(
	const std::vector<std::shared_ptr<const Statement>>& statements,
	std::map<Id, std::shared_ptr<const Expression>>& proofId_to_provenProp,
	std::map<Id, std::pair<std::vector<Id>, std::shared_ptr<const Expression>>>& definitionId_to_varsAndExpression,
	std::vector<Id>& proofIdsAdded,
	std::vector<Id>& definitionIdsAdded,
	std::vector<Id>& forAnyVarsIntroduced,
	std::vector<std::shared_ptr<const Expression>>& assumptionsIntroduced
);

struct ProofError {
	std::string message;
	FileRange fileRange;
	FileRange fileRange2;
	FileRange fileRange3;
	inline ProofError(auto&& _message, FileRange _fileRange):
		message(std::forward<decltype(_message)>(_message)),
		fileRange(_fileRange), fileRange2(FileRange::none()), fileRange3(FileRange::none()) { }
	inline ProofError(auto&& _message, FileRange _fileRange, FileRange _fileRange2):
		message(std::forward<decltype(_message)>(_message)),
		fileRange(_fileRange), fileRange2(_fileRange2), fileRange3(FileRange::none()) { }
	inline ProofError(auto&& _message, FileRange _fileRange, FileRange _fileRange2, FileRange _fileRange3):
		message(std::forward<decltype(_message)>(_message)),
		fileRange(_fileRange), fileRange2(_fileRange2), fileRange3(_fileRange3) { }
};

