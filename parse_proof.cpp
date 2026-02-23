#include "parser.h"
#include "proof.h"

[[nodiscard]] shared_ptr<const Proof> Parser::readProof_level0(
	Namespace& ns
) {
	skipWhitespace();
	FilePos startPos = currentFilePos();
	if (tryReadChar('{')) {
		Namespace ns2(&ns);
		auto [subStatements, finalProof] = readStatementsAndMaybeOneProof(ns2);
		
		if (finalProof == nullptr) {
			throw SyntaxError("Expected proof expression at end of block"sv, currentFilePos());
		}
		
		skipWhitespace();
		if (!tryReadChar('}')) {
			throw SyntaxError("Expected } to match {"sv, startPos, currentFilePos());
		}
		
		return std::make_shared<Proof_Block>(FileRange::startEnd(startPos, currentFilePos()), std::move(subStatements), std::move(finalProof));
	} else if (tryReadChar('(')) {
		auto ret = readProof(ns);
		skipWhitespace();
		if (!tryReadChar(')')) {
			throw SyntaxError("Expected ) to match ("sv, startPos, currentFilePos());
		}
		return ret;
	} else if (tryReadKeyword("substitute"sv)) {
		vector<pair<string_view, shared_ptr<const Expression>>> subsitutions;
		do {
			skipWhitespace();
			auto varName = tryReadIdentifier();
			if (!varName.has_value()) {
				throw SyntaxError("Expected identifier (variable to be substituted) or keyword 'in' in substitution list", currentFilePos());
			}
			skipWhitespace();
			if (!tryReadChar('=')) {
				throw SyntaxError("Expected = after variable name in substitution list", currentFilePos());
			}
			skipWhitespace();
			auto expr = readExpression(ns, true);
			skipWhitespace();
			subsitutions.emplace_back(varName.value(), std::move(expr));
		} while (tryReadChar(','));
		if (!tryReadKeyword("in"sv)) {
			throw SyntaxError("Expected keyword 'in' after substitution list", currentFilePos());
		}
		skipWhitespace();
		auto subProof = readProof_level0(ns);
		return std::make_shared<Proof_Substitute>(FileRange::startEnd(startPos, currentFilePos()), std::move(subsitutions), std::move(subProof));
	} else if (tryReadKeyword("unwrap"sv)) {
		auto subProof = readProof_level0(ns);
		return std::make_shared<Proof_Unwrap>(FileRange::startEnd(startPos, currentFilePos()), std::move(subProof));
	} else if (tryReadKeyword("rawunwrap"sv)) {
		auto subProof = readProof_level0(ns);
		return std::make_shared<Proof_RawUnwrap>(FileRange::startEnd(startPos, currentFilePos()), std::move(subProof));
	} else if (tryReadKeyword("wrap"sv)) {
		skipWhitespace();
		auto defName = readIdentifier("Expected identifier (definition name) after keyword 'wrap'"sv);
		auto it = ns.find(defName);
		if (!it.has_value()) {
			std::println("\nNo definition called {} exists\n", defName);
			throw "No such definition exists";
		}
		skipWhitespace();
		auto subProof = readProof_level0(ns);
		return std::make_shared<Proof_Wrap>(FileRange::startEnd(startPos, currentFilePos()), it.value(), std::move(subProof));
	} else if (tryReadKeyword("rawwrap"sv)) {
		skipWhitespace();
		auto defName = readIdentifier("Expected identifier (definition name) after keyword 'rawwrap'"sv);
		auto it = ns.find(defName);
		if (!it.has_value()) {
			std::println("\nNo definition called {} exists\n", defName);
			throw "No such definition exists";
		}
		skipWhitespace();
		auto subProof = readProof_level0(ns);
		return std::make_shared<Proof_RawWrap>(FileRange::startEnd(startPos, currentFilePos()), it.value(), std::move(subProof));
	} else {
		auto idStart = currentFilePos();
		auto idName = readIdentifier("Expected proof expression: ( or { or identifier");
		if (auto it = ns.find(idName); it.has_value()) {
			return std::make_shared<Proof_Id>(FileRange::startEnd(idStart, currentFilePos()), it.value());
		} else {
			throw SyntaxError("Unknown identifier", idStart);
		}
	}
}

[[nodiscard]] shared_ptr<const Proof> Parser::readProof(
	Namespace& ns
) {
	bool leftShoveArrows = false;
	vector<shared_ptr<const Proof>> proofExprs;
	vector<bool> arrowRawness;
	optional<FilePos> prevArrowPos;
	while (true) {
		proofExprs.push_back(readProof_level0(ns));
		skipWhitespace();
		FilePos arrowFilePos = currentFilePos();
		if (tryReadChars("<<"sv)) {
			if (!leftShoveArrows && proofExprs.size() >= 2) {
				throw SyntaxError("Cannot mix < and > operators. Use parentheses."sv, prevArrowPos.value(), arrowFilePos);
			}
			leftShoveArrows = true;
			arrowRawness.push_back(true);
		} else if (tryReadChar('<')) {
			if (!leftShoveArrows && proofExprs.size() >= 2) {
				throw SyntaxError("Cannot mix < and > operators. Use parentheses."sv, prevArrowPos.value(), arrowFilePos);
			}
			leftShoveArrows = true;
			arrowRawness.push_back(false);
		} else if (tryReadChars(">>"sv)) {
			if (leftShoveArrows && proofExprs.size() >= 2) {
				throw SyntaxError("Cannot mix < and > operators. Use parentheses."sv, prevArrowPos.value(), arrowFilePos);
			}
			leftShoveArrows = false;
			arrowRawness.push_back(true);
		} else if (tryReadChar('>')) {
			if (leftShoveArrows && proofExprs.size() >= 2) {
				throw SyntaxError("Cannot mix < and > operators. Use parentheses."sv, prevArrowPos.value(), arrowFilePos);
			}
			leftShoveArrows = false;
			arrowRawness.push_back(false);
		} else {
			break;
		}
		prevArrowPos = arrowFilePos;
	}
	
	if (leftShoveArrows) {
		shared_ptr<const Proof> ret = std::move(proofExprs[0]);
		for (unsigned int i=1; i<proofExprs.size(); i++) {
			if (arrowRawness[i-1] == false) {
				ret = std::make_shared<Proof_Shove>(
					FileRange::span(ret->fileRange, proofExprs[i]->fileRange),
					std::move(proofExprs[i]),
					std::move(ret)
				);
			} else {
				ret = std::make_shared<Proof_RawShove>(
					FileRange::span(ret->fileRange, proofExprs[i]->fileRange),
					std::move(proofExprs[i]),
					std::move(ret)
				);
			}
		}
		return ret;
	} else {
		shared_ptr<const Proof> ret = std::move(proofExprs.back());
		for (int i=proofExprs.size()-2; i>=0; i--) {
			if (arrowRawness[i] == false) {
				ret = std::make_shared<Proof_Shove>(
					FileRange::span(proofExprs[i]->fileRange, ret->fileRange),
					std::move(proofExprs[i]),
					std::move(ret)
				);
			} else {
				ret = std::make_shared<Proof_RawShove>(
					FileRange::span(proofExprs[i]->fileRange, ret->fileRange),
					std::move(proofExprs[i]),
					std::move(ret)
				);
			}
		}
		return ret;
	}
}
