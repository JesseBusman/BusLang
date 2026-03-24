#include <vector>
#include <memory>

#include "statement.h"
#include "parser.h"
#include "namespace.h"
#include "expression.h"
#include "proof.h"

using std::vector;
using std::pair;
using std::shared_ptr;

static std::optional<pair<unsigned int, Id>> functionCallGetArgCountAndCalledFunction(const Expression* expr) {
	if (auto id = dynamic_cast<const Expression_Id*>(expr)) {
		return {{0, id->id}};
	} else if (auto apply = dynamic_cast<const Expression_Apply*>(expr)) {
		auto sub = functionCallGetArgCountAndCalledFunction(apply->left.get());
		if (!sub.has_value()) return std::nullopt;
		return {{sub->first+1, sub->second}};
	} else {
		return std::nullopt;
	}
}

[[nodiscard]] pair<vector<shared_ptr<const Statement>>, shared_ptr<const Proof>> Parser::readStatementsAndMaybeOneProof(
	Namespace& ns
) {
	vector<shared_ptr<const Statement>> ret;
	while (true) {
		skipWhitespace();
		
		if (areAtEnd() || tryPeekChar(')')) {
			return {std::move(ret), nullptr};
		}
		
		if (tryReadKeyword("syntax"sv)) {
			Namespace ns2(&ns);
			
			vector<Id> syntaxNames;
			{
				auto savedPos = currentFilePos();
				
				skipWhitespace();
				while (!tryReadChar('(')) {
					skipWhitespace();
					auto startPos = currentFilePos();
					auto syntaxPieceName = readIdentifier("Expected ( or syntax piece name followed by : ("sv);
					skipWhitespace();
					readChar(':', "Expected :"sv);
					Id syntaxPieceId = ns2.make(syntaxPieceName, FileRange::startEnd(startPos, currentFilePos()));
					syntaxNames.push_back(syntaxPieceId);
					skipWhitespace();
					skipParenEnclosedStuff();
					skipWhitespace();
					readChar(',', "Expected ,"sv);
					skipWhitespace();
				}
				
				rewindTo(savedPos);
			}
			
			map<
				Id,
				vector<pair<
					vector<std::variant<Id, char, Space, pair<Id, Id>>>,
					shared_ptr<const Expression>
				>>
			> syntaxId_to_piecesAndExpr;
			
			while (true) {
				skipWhitespace();
				if (tryReadChar('(')) break;
				auto syntaxPieceName = readIdentifier("Expected ( or syntax piece name followed by : ("sv);
				
				optional<Id> syntaxPieceId;
				for (auto& sn : syntaxNames) {
					if (sn.name == syntaxPieceName) { syntaxPieceId = sn; break; }
				}
				if (!syntaxPieceId.has_value()) throw 12345;
				
				skipWhitespace();
				readChar(':', "Expected :"sv);
				skipWhitespace();
				readChar('(', "Expected ("sv);
				
				vector<pair<
					vector<std::variant<Id, char, Space, pair<Id, Id>>>,
					shared_ptr<const Expression>
				>> syntaxVariants;
				while (true) {
					Namespace ns3(&ns2);
					skipWhitespace();
					readChar('(', "Expected ("sv);
					skipWhitespace();
					auto parts = readSyntaxPieces(ns3);
					skipWhitespace();
					readChar(')', "Expected )"sv);
					skipWhitespace();
					readChar('=', "Expected ="sv);
					auto expr = readExpression(ns3, ',');
					syntaxVariants.emplace_back(std::make_pair(std::move(parts), std::move(expr)));
					skipWhitespace();
					if (tryReadChar(')')) break;
					readChar(',', "Expected ) or ,"sv);
					skipWhitespace();
					if (tryReadChar(')')) break;
				}
				syntaxId_to_piecesAndExpr.emplace(std::make_pair(syntaxPieceId.value(), std::move(syntaxVariants)));
				
				skipWhitespace();
				readChar(',', "Expected ,"sv);
			}
			skipWhitespace();
			
			Namespace ns3(&ns2);
			auto parts = readSyntaxPieces(ns3);
			
			if (parts.size() == 0) throw SyntaxError("Main syntax must not be empty"sv, currentFilePos());
			
			skipWhitespace();
			readChar(')', "Expected )"sv);
			skipWhitespace();
			readChar('=', "Expected ="sv);
			auto expr = readExpression(ns3, ',');
			
			skipWhitespace();
			
			readChar(',', "Expected , followed by 'precedence'");
			skipWhitespace();
			readKeyword("precedence"sv, "Expected keyword 'precedence'"sv);
			skipWhitespace();
			optional<long> precedence;
			if (tryReadKeyword("reset"sv)) {
				precedence = std::nullopt;
			} else {
				precedence = readInteger("Expected 'reset' or integer specifying precedence"sv);
			}
			skipWhitespace();
			readChar(',', "Expected , followed by 'associativity'");
			skipWhitespace();
			readKeyword("associativity"sv, "Expected keyword 'associativity'"sv);
			skipWhitespace();
			Associativity associativity;
			if (tryReadKeyword("left"sv)) associativity = LEFT;
			else if (tryReadKeyword("right"sv)) associativity = RIGHT;
			else if (tryReadKeyword("noassoc"sv)) associativity = NOASSOC;
			else throw SyntaxError("Expected left/right/noassoc"sv, currentFilePos());
			skipWhitespace();
			readChar(';', "Expected ;"sv);
			
			ns.addSyntax(Syntax(
				std::move(syntaxId_to_piecesAndExpr), std::move(parts), std::move(expr), precedence, associativity
			));
		} else if (tryReadKeyword("scope"sv)) {
			skipWhitespace();
			
			FilePos openingBracePos = currentFilePos();
			readChar('(', "Expected ( after keyword 'scope'"sv);
			
			Namespace ns2(&ns);
			auto [statements, endExpr] = readStatementsAndMaybeOneProof(ns2);
			
			if (!tryReadChar(')')) {
				throw SyntaxError("Expected ) to match ("sv, openingBracePos, currentFilePos());
			}
			
			readChar(';', "Expected ; at end of scope block"sv);
			
			skipWhitespace();
			
			if (endExpr != nullptr) throw SyntaxError("Proof at end of block is not allowed in scope block"sv, endExpr->fileRange);
			
			ret.push_back(std::make_shared<Statement_Block>(std::move(statements)));
		} else if (tryReadKeyword("define"sv)) {
			skipWhitespace();
			
			FilePos defNameStart = currentFilePos();
			auto defName = tryReadIdentifier();
			if (!defName.has_value()) {
				throw SyntaxError("Expected identifier (definition name) after keyword 'define'"sv, currentFilePos());
			}
			FilePos defNameEnd = currentFilePos();
			
			skipWhitespace();
			
			auto defId = ns.make(defName.value(), FileRange::startEnd(defNameStart, defNameEnd));
			
			auto start = currentFilePos();
			if (tryReadChar('(')) {
				// It's a pattern-matching style definition
				
				vector<pair<pair<vector<Id>, shared_ptr<const Expression>>, shared_ptr<const Expression>>> patternsAndValues;
				
				std::optional<unsigned int> argCount;
				
				do {
					skipWhitespace();
					if (tryPeekChar(')')) break; // Allow trailing comma
					auto lhsStartPos = currentFilePos();
					auto lhs = readExpression(ns, '=');
					vector<Id> vars;
					if (auto fa = dynamic_cast<const Expression_ForAny*>(lhs.get())) {
						vars = std::move(fa->varIds);
						shared_ptr<const Expression> newLhs = std::move(fa->subExpr);
						lhs = std::move(newLhs);
					}
					
					auto argCountAndFuncId = functionCallGetArgCountAndCalledFunction(lhs.get());
					if (!argCountAndFuncId.has_value()) throw SyntaxError("Invalid syntax in definition pattern"sv, FileRange::startEnd(lhsStartPos, currentFilePos()));
					if (argCountAndFuncId->second != defId) throw SyntaxError("Invalid syntax in definition pattern: left hand side must be a function call on the definition name"sv, FileRange::startEnd(lhsStartPos, currentFilePos()));
					if (argCount.has_value() && argCount.value() != argCountAndFuncId->first) throw SyntaxError("Invalid syntax in definition pattern: left hand sides must have same amount of arguments"sv, FileRange::startEnd(lhsStartPos, currentFilePos()));
					
					skipWhitespace();
					readChar('=', "Expected = after pattern"sv);
					Namespace ns2(&ns);
					for (const auto& var : vars) ns2.add(var);
					auto rhs = readExpression(ns2, ',');
					skipWhitespace();
					patternsAndValues.emplace_back(std::make_pair(std::move(vars), std::move(lhs)), std::move(rhs));
				} while (tryReadChar(','));
				
				if (patternsAndValues.size() == 0) throw SyntaxError("Definition must have at least one pattern"sv, start);
				
				skipWhitespace();
				readChar(')', "Expected ) at end of define pattern list"sv);
				skipWhitespace();
				readChar(';', "Expected ; after definition"sv);
				skipWhitespace();
				
				ret.push_back(std::make_shared<Statement_Define>(defId, std::move(patternsAndValues)));
			} else {
				// It's a normal direct definition
				
				Namespace ns2(&ns);
				
				vector<Id> varIds;
				
				while (!tryReadChar('=')) {
					auto defVarPos = currentFilePos();
					auto defVarName = tryReadIdentifier();
					if (!defVarName.has_value()) {
						throw SyntaxError("Expected definition variable name or ="sv, currentFilePos());
					}
					for (auto& v : varIds) {
						if (v.name == defVarName) {
							throw SyntaxError("Definition variable name appears multiple times"sv, defVarPos); // TODO also point at the previous variable
						}
					}
					skipWhitespace();
					
					varIds.push_back(ns2.make(defVarName.value(), FileRange::startEnd(defVarPos, currentFilePos())));
				} 
				skipWhitespace();
				auto defRhs = readExpression(ns2, ';');
				
				skipWhitespace();
				if (!tryReadChar(';')) {
					throw SyntaxError("Expected ; after definition", currentFilePos());
				}
				skipWhitespace();
				
				vector<pair<pair<vector<Id>, shared_ptr<const Expression>>, shared_ptr<const Expression>>> patternsAndValues;
				
				shared_ptr<const Expression> lhs = std::make_shared<Expression_Id>(FileRange::startEnd(defNameStart, defNameEnd), defId);
				for (const auto& varId : varIds) {
					lhs = std::make_shared<Expression_Apply>(
						FileRange::none(),
						std::move(lhs),
						std::make_shared<Expression_Id>(FileRange::none(), varId)
					);
				}
				patternsAndValues.emplace_back(std::make_pair(std::move(varIds), std::move(lhs)), std::move(defRhs));
				
				ret.push_back(std::make_shared<Statement_Define>(defId, std::move(patternsAndValues)));
			}
		} else if (tryReadKeyword("atom"sv)) {
			vector<Id> atomIds;
			do {
				skipWhitespace();
				auto atomNameStart = currentFilePos();
				auto atomName = tryReadIdentifier();
				if (!atomName.has_value()) {
					throw SyntaxError("Expected identifier (atom name) after keyword 'atom'", currentFilePos());
				}
				Id atomId = ns.make(atomName.value(), FileRange::startEnd(atomNameStart, currentFilePos()));
				atomIds.push_back(atomId);
				skipWhitespace();
			} while (tryReadChar(','));
			
			readChar(';', "Expected ; after atom declaration"sv);
			
			ret.push_back(std::make_shared<Statement_Atoms>(std::move(atomIds)));
		} else if (tryReadKeyword("print"sv)) {
			skipWhitespace();
			auto proof = readProof(ns);
			skipWhitespace();
			readChar(';', "Expected ; after print statement"sv);
			ret.push_back(std::make_shared<Statement_Print>(std::move(proof)));
		} else if (tryReadKeyword("musterror"sv)) {
			skipWhitespace();
			auto proof = readProof(ns);
			skipWhitespace();
			readChar(';', "Expected ; after musterror statement"sv);
			ret.push_back(std::make_shared<Statement_MustError>(std::move(proof)));
		} else if (tryReadKeyword("assume"sv)) {
			skipWhitespace();
			auto assumptionNameStart = currentFilePos();
			auto assumptionName = readIdentifier("Expected identifier (assumption name) after keyword 'assume'"sv);
			auto assumptionNameEnd = currentFilePos();
			skipWhitespace();
			readKeyword("proves"sv, "Expected keyword 'proves' after assumption name"sv);
			skipWhitespace();
			auto assumedProposition = readExpression(ns, (char)0x00);
			skipWhitespace();
			readChar(';', "Expected ; after assumed proposition"sv);
			skipWhitespace();
			
			Id assumptionId = ns.make(assumptionName, FileRange::startEnd(assumptionNameStart, assumptionNameEnd));
			
			ret.push_back(std::make_shared<Statement_Assume>(assumptionId, std::move(assumedProposition)));
		} else if (tryReadKeyword("require"sv)) {
			skipWhitespace();
			auto requirementNameStart = currentFilePos();
			auto requirementName = readIdentifier("Expected identifier (requirement name) after keyword 'require'"sv);
			auto requirementNameEnd = currentFilePos();
			skipWhitespace();
			readKeyword("proves"sv, "Expected keyword 'proves' after requirement name"sv);
			skipWhitespace();
			
			auto requiredProposition = readExpression(ns, (char)0x00);
			skipWhitespace();
			readKeyword("by"sv, "Expected keyword 'by' after required proposition"sv);
			skipWhitespace();
			auto proof = readProof(ns);
			skipWhitespace();
			readChar(';', "Expected ; after proof"sv);
			skipWhitespace();
			
			Id requirementId = ns.make(requirementName, FileRange::startEnd(requirementNameStart, requirementNameEnd));
			
			ret.push_back(std::make_shared<Statement_Require>(requirementId, std::move(requiredProposition), std::move(proof)));
		} /*else if (tryReadKeyword("equivalent"sv)) {
			skipWhitespace();
			
			vector<vector<shared_ptr<const Statement>>> statementss;
			
			auto openSquareBracketPos = currentFilePos();
			readChar('[', "Expected [ after keyword 'equivalent'"sv);
			do {
				skipWhitespace();
				auto openBracePos = currentFilePos();
				readChar('{', "Expected { to start block in equivalent list"sv);
				Namespace ns2(&ns);
				auto [statements, endProofExpr] = readStatementsAndMaybeOneProof(ns2);
				skipWhitespace();
				if (!tryReadChar('}')) throw SyntaxError("Expected } to match {"sv, currentFilePos(), openBracePos);
				if (endProofExpr != nullptr) throw SyntaxError("Proof at end of block not allowed"sv, endProofExpr->fileRange);
				skipWhitespace();
				
				statementss.emplace_back(std::move(statements));
			} while (tryReadChar(','));
			if (!tryReadChar(']')) throw SyntaxError("Expected ] to match ["sv, currentFilePos(), openSquareBracketPos);
			skipWhitespace();
			readChar(';', "Expected ; at end of 'equivalent' statement"sv);
			
			ret.push_back(std::make_shared<Statement_Equivalent>(std::move(statementss)));
		}*/ else if (tryReadKeyword("forany"sv)) {
			vector<Id> varIds;
			do {
				skipWhitespace();
				auto varNameStart = currentFilePos();
				auto varName = tryReadIdentifier();
				if (!varName.has_value()) {
					throw SyntaxError("Expected identifier (variable name) or : in forany variable list", currentFilePos());
				}
				
				varIds.push_back(ns.make(varName.value(), FileRange::startEnd(varNameStart, currentFilePos())));
				skipWhitespace();
			} while (tryReadChar(','));
			readChar(':', "Expected : after forany variable list"sv);
			skipWhitespace();
			
			ret.push_back(std::make_shared<Statement_ForAny>(std::move(varIds)));
		} else {
			FilePos exprStart = currentFilePos();
			auto proof = readProof(ns);
			
			skipWhitespace();
			
			if (!tryPeekChar(')')) {
				throw SyntaxError("Expected ). Proof is only allowed at end of block"sv, currentFilePos(), exprStart);
			} else {
				return {std::move(ret), std::move(proof)};
			}
		}
	}
}
