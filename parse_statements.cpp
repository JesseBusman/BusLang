#include <variant>
#include <vector>
#include <sstream>
#include <memory>

#include "statement.h"
#include "parser.h"
#include "namespace.h"
#include "expression.h"
#include "proof.h"
#include "proofsyntax.h"

using std::vector;
using std::pair;
using std::shared_ptr;


[[nodiscard]] shared_ptr<const Proof> Parser::readStatementsAndMaybeOneProof(
	Namespace& ns,
	vector<shared_ptr<const Statement>>& ret,
	bool syntaxAssumePermitted
) {
	static unsigned long assumeProofsyntaxCounter = 0;
	while (true) {
		skipWhitespace();
		
		std::println("readStatementsAndMaybeOneProof has remaining: '{}'...", str.substr(0, 100));
		
		if (areAtEnd() || tryPeekChar(')')) {
			return nullptr;
		}
		
		if (tryReadKeyword("proofsyntax"sv)) {
			ns.addProofSyntax(std::move(*readProofSyntax(false)));
		} else if (tryReadKeyword("syntax"sv)) {
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
					skipParenEnclosedStuff(false);
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
			vector<shared_ptr<const Statement>> statements;
			auto endExpr = readStatementsAndMaybeOneProof(ns2, statements, syntaxAssumePermitted);
			
			if (!tryReadChar(')')) {
				throw SyntaxError("Expected ) to match ("sv, openingBracePos, currentFilePos());
			}
			
			readChar(';', "Expected ; at end of scope block"sv);
			
			skipWhitespace();
			
			if (endExpr != nullptr) throw SyntaxError("Proof at end of block is not allowed in scope block"sv, endExpr->fileRange);
			
			ret.push_back(std::make_shared<Statement_Block>(std::move(statements)));
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
			auto proof = readProof(ns, ';', syntaxAssumePermitted);
			//skipWhitespace();
			readChar(';', "Expected ; after print statement"sv);
			ret.push_back(std::make_shared<Statement_Print>(std::move(proof)));
		} else if (tryReadKeyword("musterror"sv)) {
			skipWhitespace();
			auto proof = readProof(ns, ';', syntaxAssumePermitted);
			//skipWhitespace();
			readChar(';', "Expected ; after musterror statement"sv);
			ret.push_back(std::make_shared<Statement_MustError>(std::move(proof)));
		} else if (tryReadKeyword("syntaxassume"sv)) {
			if (!syntaxAssumePermitted) throw SyntaxError("syntaxssume is only permitted in an 'assume proofsyntax'"sv, currentFilePos());
			skipWhitespace();
			auto assumptionNameStart = currentFilePos();
			auto assumptionName = readIdentifier("Expected identifier (assumption name) after keyword 'syntaxassume'"sv);
			auto assumptionNameEnd = currentFilePos();
			skipWhitespace();
			readKeyword("proves"sv, "Expected keyword 'proves' after assumption name"sv);
			skipWhitespace();
			auto assumedProposition = readExpression(ns, (char)0x00);
			skipWhitespace();
			readChar(';', "Expected ; after assumed proposition"sv);
			skipWhitespace();
			
			Id assumptionId = ns.make(assumptionName, FileRange::startEnd(assumptionNameStart, assumptionNameEnd));
			
			ret.push_back(std::make_shared<Statement_SyntaxAssume>(assumptionId, std::move(assumedProposition)));
		} else if (tryReadKeyword("assume"sv)) {
			skipWhitespace();
			if (tryReadKeyword("proofsyntax"sv)) {
				auto pos = currentFilePos();
				ns.addProofSyntax(std::move(*readProofSyntax(true)));
				string_view name = *new std::string("assume_proofsyntax_"s + std::to_string(pos.line) + "_" + std::to_string(pos.col)+"_"+std::to_string(assumeProofsyntaxCounter));
				ret.push_back(std::make_shared<Statement_Assume>(
					// TODO don't leak
					Id::make(name),
					std::make_shared<Expression_Id>(FileRange::startEnd(pos, currentFilePos()), ns.make(name, FileRange::startEnd(pos, currentFilePos())))
				));
			} else {
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
			}
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
			auto proof = readProof(ns, ';', syntaxAssumePermitted);
			//skipWhitespace();
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
		}*/
		else if (tryReadKeyword("forany"sv)) {
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
			auto statementStartPos = currentFilePos();
			auto str_ = readUntilCharOrEnd(';', true);
			if (tryReadChar(';')) {
				auto statementEndPos = currentFilePos();
				
				auto str = string_view(&str_[0], str_.size()+1);
				Parser parser(str);
				//parser.line = filePos.line;
				//parser.col = filePos.col;
				//parser.index = TODO
				
				std::println("statement = '{}'", str);
				
				auto trim = [](string_view sv)noexcept->string_view{
					string_view ret = sv;
					while (ret.size() != 0 && (ret[0] == ' ' || ret[0] == '\t' || ret[0] == '\n' || ret[0] == '\r')) ret = ret.substr(1);
					while (ret.size() != 0 && (ret.back() == ' ' || ret.back() == '\t' || ret.back() == '\n' || ret.back() == '\r')) ret = ret.substr(0, ret.size()-1);
					return ret;
				};
				
				parser.skipWhitespace();
				auto startPos = parser.currentFilePos();
				for (auto& proofSyntax : ns.getProofSyntaxes()) {
					
					
					span<const ProofSyntax_OutputSegment> outputSegments;
					if (proofSyntax.mainParseDefinition.size() >= 2 &&
					    std::holds_alternative<ProofSyntax_ParsePiece_Typed>(proofSyntax.mainParseDefinition[proofSyntax.mainParseDefinition.size()-1]) &&
					    std::holds_alternative<ProofSyntax_Type_Any>(std::get<ProofSyntax_ParsePiece_Typed>(proofSyntax.mainParseDefinition[proofSyntax.mainParseDefinition.size()-1]).type) &&
					    std::holds_alternative<ProofSyntax_ParsePiece_Semicolon>(proofSyntax.mainParseDefinition[proofSyntax.mainParseDefinition.size()-2])
					) {
						if (
							proofSyntax.mainOutput.size() >= 1 &&
							std::holds_alternative<ProofSyntax_OutputSegment_Variable>(proofSyntax.mainOutput.back()) &&
							std::get<ProofSyntax_OutputSegment_Variable>(proofSyntax.mainOutput.back()).varName == std::get<ProofSyntax_ParsePiece_Typed>(proofSyntax.mainParseDefinition[proofSyntax.mainParseDefinition.size()-1]).name
						) {
							outputSegments = span(proofSyntax.mainOutput).subspan(0, proofSyntax.mainOutput.size()-1);
						} else if (
							proofSyntax.mainOutput.size() >= 2 &&
							std::holds_alternative<ProofSyntax_OutputSegment_Variable>(proofSyntax.mainOutput[proofSyntax.mainOutput.size()-2]) &&
							std::get<ProofSyntax_OutputSegment_Variable>(proofSyntax.mainOutput[proofSyntax.mainOutput.size()-2]).varName == std::get<ProofSyntax_ParsePiece_Typed>(proofSyntax.mainParseDefinition[proofSyntax.mainParseDefinition.size()-1]).name &&
							std::holds_alternative<ProofSyntax_OutputSegment_Literal>(proofSyntax.mainOutput.back()) &&
							trim(std::get<ProofSyntax_OutputSegment_Literal>(proofSyntax.mainOutput.back()).strv()).size() == 0
						) {
							outputSegments = span(proofSyntax.mainOutput).subspan(0, proofSyntax.mainOutput.size()-2);
						} else {
							continue;
						}
					} else {
						continue;
					}
					
					auto proofSyntaxSubstitute = [](span<const ProofSyntax_OutputSegment> outputSegments, const ProofSyntax& proofSyntax, const map<string_view, ProofSyntax_MatchedValue>& matches)->std::string{
						std::stringstream newString;
						for (auto& outputSegment : outputSegments) {
							getOutputSegmentStr(outputSegment, proofSyntax, matches, newString);
						}
						return std::move(newString).str();
					};
					
					map<string_view, ProofSyntax_MatchedValue> matches;
					if (tryReadProofByCustomSyntax(parser, ns, span(proofSyntax.mainParseDefinition).subspan(0, proofSyntax.mainParseDefinition.size()-1), proofSyntax, matches, {NothingHere{}}, 0)) {
						parser.skipWhitespace();
						if (parser.areAtEnd()) {
							
							
							
							std::string* newString_ = new std::string(proofSyntaxSubstitute(outputSegments, proofSyntax, matches)); // TODO: leak
							//std::print("isAssumed={}\n\n{}\n", proofSyntax.isAssumed, *newString_);
							std::print("\nCustom proofsyntax gave:\n{}\n", *newString_);
							
							Parser subParser(*newString_);
							auto proofAtEnd = subParser.readStatementsAndMaybeOneProof(ns, ret, proofSyntax.isAssumed);
							
							if (proofAtEnd != nullptr) {
								throw SyntaxError("Proof is only allowed at end of block"sv, currentFilePos());
							} else {
								goto good;
							}
						} else {
							parser.rewindTo(startPos);
						}
					} else {
						parser.rewindTo(startPos);
					}
				}
				throw SyntaxError("Could not parse statement"sv, FileRange::startEnd(statementStartPos, statementEndPos));
				
				good:;
			} else {
				rewindTo(statementStartPos);
				FilePos exprStart = currentFilePos();
				auto proof = readProof(ns, ')', syntaxAssumePermitted);
				//skipWhitespace();
				if (!tryPeekChar(')')) {
					throw SyntaxError("Expected ). Proof is only allowed at end of block"sv, currentFilePos(), exprStart);
				} else {
					return proof;
				}
			}
		}
	}
}
