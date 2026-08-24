module;

#include <stdint.h>
#include <climits>
#include <strings.h>

module BusLang;

import std;
import :Parser;
import :Id;
import :Expression;
import :Syntax;
import :Namespace;


using std::shared_ptr;
using std::pair;
using std::vector;
using std::map;
using std::span;
using std::optional;
using std::string_literals::operator""s;
using std::string_view_literals::operator""sv;

#define EXPR_PARSER_LOG false

struct ParseRegions {
	span<const std::variant<shared_ptr<const Expression>, char, Space>> exprParts;
	span<const std::variant<Id, char, Space, pair<Id, Id>>> syntax;
	
	constexpr bool operator == (const ParseRegions& other) const noexcept {
		return
			this->exprParts.data() == other.exprParts.data() &&
			this->exprParts.size() == other.exprParts.size() &&
			this->syntax   .data() == other.syntax   .data() &&
			this->syntax   .size() == other.syntax   .size();
	}
	
	constexpr bool operator < (const ParseRegions& other) const noexcept {
		if (this->exprParts.data() < other.exprParts.data()) return true;
		if (this->exprParts.data() > other.exprParts.data()) return false;
		if (this->exprParts.size() < other.exprParts.size()) return true;
		if (this->exprParts.size() > other.exprParts.size()) return false;
		if (this->syntax   .data() < other.syntax   .data()) return true;
		if (this->syntax   .data() > other.syntax   .data()) return false;
		if (this->syntax   .size() < other.syntax   .size()) return true;
		if (this->syntax   .size() > other.syntax   .size()) return false;
		return true;
	}
};

[[nodiscard]] static optional<shared_ptr<const Expression>> parseExpression(
	span<const std::variant<shared_ptr<const Expression>, char, Space>> exprParts,
	const Namespace& ns,
	long minimumPrecedence,
	std::set<ParseRegions>& spansFailedToParseCache,
	int logDepth=1
);

[[nodiscard]] static vector<map<Id, shared_ptr<const Expression>>> getPartialParseOptions(
	span<const std::variant<shared_ptr<const Expression>, char, Space>> exprParts,
	span<const std::variant<Id, char, Space, pair<Id, Id>>> syntax,
	const map<Id, vector<pair<vector<std::variant<Id, char, Space, pair<Id, Id>>>, shared_ptr<const Expression>>>>& name_to_syntaxAndExpr,
	const Namespace& ns,
	Associativity currentAssociativity,
	long minimumPrecedence,
	unsigned int maxReturnedOptions,
	std::set<ParseRegions>& spansFailedToParseCache,
	int logDepth
) {
	vector<map<Id, shared_ptr<const Expression>>> ret;
	
	if (spansFailedToParseCache.contains({exprParts, syntax})) return ret;
	
	#if EXPR_PARSER_LOG
	for (int i=0; i<logDepth; i++) std::print("|  ");
	std::print("getPartialParseOptions   exprParts=(");
	for (auto& part : exprParts) {
		if (std::holds_alternative<char>(part)) std::print("{}", std::get<char>(part));
		else if (std::holds_alternative<Space>(part)) std::print(" _ ");
		else if (std::holds_alternative<shared_ptr<const Expression>>(part)) { std::print(" ("); std::get<shared_ptr<const Expression>>(part)->print(); std::print(") "); }
		else throw 347457;
	}
	std::print(")   syntax=(");
	for (auto& s : syntax) {
		if (std::holds_alternative<char>(s)) std::print("{}", std::get<char>(s));
		else if (std::holds_alternative<Space>(s)) std::print(" _ ");
		else if (std::holds_alternative<Id>(s)) std::print(" {} ", std::get<Id>(s).name);
		else if (std::holds_alternative<pair<Id, Id>>(s)) std::print(" {}:{} ", std::get<pair<Id, Id>>(s).first.name, std::get<pair<Id, Id>>(s).second.name);
		else throw 8732253;
	}
	std::println(")");
	#endif
	
	auto matchesOperator = [](span<const char> operatorChars, span<const std::variant<shared_ptr<const Expression>, char, Space>> exprParts) -> bool {
		if (operatorChars.size() > exprParts.size()) return false;
		for (unsigned int j=0; j<operatorChars.size(); j++) {
			if (!std::holds_alternative<char>(exprParts[j])) return false;
			if (std::get<char>(exprParts[j]) != operatorChars[j]) return false;
		}
		return true;
	};
	
	
	if (exprParts.size() == 0 && syntax.size() == 0) {
		// Empty pattern matches empty expression
		ret.emplace_back();
	} else if (syntax.size() != 0 && std::holds_alternative<Space>(syntax[0])) {
		ret = getPartialParseOptions(exprParts, syntax.subspan(1), name_to_syntaxAndExpr, ns, currentAssociativity, minimumPrecedence, maxReturnedOptions, spansFailedToParseCache, logDepth+1);
	} else if (exprParts.size() != 0 && std::holds_alternative<Space>(exprParts[0])) {
		ret = getPartialParseOptions(exprParts.subspan(1), syntax, name_to_syntaxAndExpr, ns, currentAssociativity, minimumPrecedence, maxReturnedOptions, spansFailedToParseCache, logDepth+1);
	} else if (syntax.size() == 0) {
		// Does not match
	} else if (exprParts.size() == 0) {
		// Does not match
	} else if (std::holds_alternative<char>(syntax[0])) {
		std::vector<char> operatorChars;
		for (unsigned int i=0; i<syntax.size() && std::holds_alternative<char>(syntax[i]); i++) {
			operatorChars.push_back(std::get<char>(syntax[i]));
		}
		
		if (matchesOperator(operatorChars, exprParts)) {
			ret = getPartialParseOptions(exprParts.subspan(operatorChars.size()), syntax.subspan(operatorChars.size()), name_to_syntaxAndExpr, ns, currentAssociativity, minimumPrecedence, maxReturnedOptions, spansFailedToParseCache, logDepth+1);
		}
	} else if (std::holds_alternative<Id>(syntax[0]) || std::holds_alternative<pair<Id, Id>>(syntax[0])) {
		if (syntax.size() == 1) {
			if (std::holds_alternative<Id>(syntax[0])) {
				auto maybeExpr = parseExpression(exprParts, ns, currentAssociativity == RIGHT ? minimumPrecedence : minimumPrecedence+1, spansFailedToParseCache, logDepth+1);
				if (maybeExpr.has_value()) {
					ret.emplace_back(map<Id, shared_ptr<const Expression>>{{std::get<Id>(syntax[0]), std::move(maybeExpr.value())}});
				}
			} else if (std::holds_alternative<pair<Id, Id>>(syntax[0])) {
				auto [param, subSyntax] = std::get<pair<Id, Id>>(syntax[0]);
				auto it = name_to_syntaxAndExpr.find(subSyntax);
				if (it == name_to_syntaxAndExpr.end()) throw 911923;
				
				for (auto& syntaxVariant : it->second) {
					auto options = getPartialParseOptions(exprParts, syntaxVariant.first, name_to_syntaxAndExpr, ns, currentAssociativity, minimumPrecedence, maxReturnedOptions, spansFailedToParseCache, logDepth+1); // TODO: currentAssociativity & minimumPrecedence
					for (auto& option : options) {
						ret.emplace_back(map<Id, shared_ptr<const Expression>>{{param, syntaxVariant.second->substitute(option)}});
					}
					if (ret.size() >= maxReturnedOptions) break;
				}
			} else {
				throw 98423;
			}
		} else if (syntax.size() == 2 && std::holds_alternative<Id>(syntax[0]) && std::holds_alternative<Id>(syntax[1])) {
			// Only the built-in 'apply' syntax may have an expression followed by an expression, without any operator char in between
			for (int i=exprParts.size()-1; i>=1; i--) {
				auto maybeLhs = parseExpression(exprParts.subspan(0, i), ns, minimumPrecedence, spansFailedToParseCache, logDepth+1);
				if (maybeLhs.has_value()) {
					auto maybeRhs = parseExpression(exprParts.subspan(i), ns, minimumPrecedence+1, spansFailedToParseCache, logDepth+1);
					if (maybeRhs.has_value()) {
						ret.emplace_back(map<Id, shared_ptr<const Expression>>{
							{std::get<Id>(syntax[0]), std::move(maybeLhs.value())},
							{std::get<Id>(syntax[1]), std::move(maybeRhs.value())}
						});
						if (ret.size() >= maxReturnedOptions) break;
					}
				}
			}
		} else {
			std::vector<char> operatorChars;
			for (unsigned int i=1; i<syntax.size() && std::holds_alternative<char>(syntax[i]); i++) {
				operatorChars.push_back(std::get<char>(syntax[i]));
			}
			
			if (operatorChars.size() == 0) throw 184871282;
			
			// Find operators
			std::vector<unsigned int> operatorSpots;
			
			if (currentAssociativity == LEFT) {
				for (int i=exprParts.size()-operatorChars.size(); i >= 1; i--) {
					if (matchesOperator(operatorChars, exprParts.subspan(i))) {
						operatorSpots.push_back(i);
					}
				}
			} else if (currentAssociativity == RIGHT || currentAssociativity == NOASSOC) {
				for (unsigned int i=1; i+operatorChars.size()-1<exprParts.size(); i++) {
					if (matchesOperator(operatorChars, exprParts.subspan(i))) {
						operatorSpots.push_back(i);
					}
				}
			} else {
				throw 8732432;
			}
			
			#if EXPR_PARSER_LOG
			for (int i=0; i<logDepth; i++) std::print("|  ");
			std::println("{} operator spots for {}", operatorSpots.size(), std::string_view(&operatorChars[0], operatorChars.size()));
			#endif
			
			for (auto spot : operatorSpots) {
				if (ret.size() >= maxReturnedOptions) break;
				if (std::holds_alternative<Id>(syntax[0])) {
					auto maybeExpr = parseExpression(exprParts.subspan(0, spot), ns, currentAssociativity == LEFT ? minimumPrecedence : minimumPrecedence+1, spansFailedToParseCache, logDepth+1);
					if (!maybeExpr.has_value()) continue;
					auto options = getPartialParseOptions(exprParts.subspan(spot + operatorChars.size()), syntax.subspan(1 + operatorChars.size()), name_to_syntaxAndExpr, ns, currentAssociativity, minimumPrecedence, maxReturnedOptions-ret.size(), spansFailedToParseCache, logDepth+1);
					for (auto& option : options) {
						auto& slot = option[std::get<Id>(syntax[0])];
						if (slot != nullptr) throw 987127812;
						slot = maybeExpr.value();
						ret.emplace_back(std::move(option));
					}
				} else if (std::holds_alternative<pair<Id, Id>>(syntax[0])) {
					auto [param, subSyntax] = std::get<pair<Id, Id>>(syntax[0]);
					auto it = name_to_syntaxAndExpr.find(subSyntax);
					if (it == name_to_syntaxAndExpr.end()) throw 3498243;
					auto options = getPartialParseOptions(exprParts.subspan(spot + operatorChars.size()), syntax.subspan(1 + operatorChars.size()), name_to_syntaxAndExpr, ns, currentAssociativity, minimumPrecedence, UINT32_MAX, spansFailedToParseCache, logDepth+1);
					if (options.size() != 0) {
						for (auto& syntaxVariant : it->second) {
							auto lhsOptions = getPartialParseOptions(exprParts.subspan(0, spot), syntaxVariant.first, name_to_syntaxAndExpr, ns, currentAssociativity, minimumPrecedence, UINT32_MAX, spansFailedToParseCache, logDepth+1); // TODO: currentAssociativity & minimumPrecedence
							for (const auto& lhsOption : lhsOptions) {
								auto lhsExpr = syntaxVariant.second->substitute(lhsOption);
								for (const auto& option : options) {
									auto newMap = option;
									auto& slot = newMap[param];
									if (slot != nullptr) throw 8348234234;
									slot = lhsExpr;
									ret.emplace_back(std::move(newMap));
									if (ret.size() >= maxReturnedOptions) break;
								}
								if (ret.size() >= maxReturnedOptions) break;
							}
							if (ret.size() >= maxReturnedOptions) break;
						}
					}
				} else {
					throw 92398243;
				}
			}
		}
	} else {
		throw 8723123;
	}
	
	#if EXPR_PARSER_LOG
	for (int i=0; i<logDepth; i++) std::print("|  ");
	std::println("{} options", ret.size());
	#endif
	
	if (ret.size() == 0) {
		spansFailedToParseCache.insert({exprParts, syntax});
	}
	
	return ret;
}

[[nodiscard]] static optional<shared_ptr<const Expression>> parseExpression(
	span<const std::variant<shared_ptr<const Expression>, char, Space>> exprParts,
	const Namespace& ns,
	long minimumPrecedence,
	std::set<ParseRegions>& spansFailedToParseCache,
	int logDepth
) {
	if (exprParts.size() == 1 && std::holds_alternative<shared_ptr<const Expression>>(exprParts[0])) {
		return std::get<shared_ptr<const Expression>>(exprParts[0]);
	}
	
	#if EXPR_PARSER_LOG
	for (int i=0; i<logDepth; i++) std::print("|  ");
	std::print("parseExpression(");
	for (auto& part : exprParts) {
		if (std::holds_alternative<char>(part)) std::print("{}", std::get<char>(part));
		else if (std::holds_alternative<Space>(part)) std::print(" _ ");
		else if (std::holds_alternative<shared_ptr<const Expression>>(part)) { std::print(" ("); std::get<shared_ptr<const Expression>>(part)->print(); std::print(") "); }
		else throw 8725324;
	}
	std::println(")    minimumPrecedence={}", minimumPrecedence);
	#endif
		
	shared_ptr<const Expression> ret = nullptr;
	long retPrecedence = LONG_MIN;
	
	for (auto& syntax : ns.getSyntaxes()) {
		if (syntax.precedence.has_value() && syntax.precedence < minimumPrecedence) continue;
		auto partialParseOptions = getPartialParseOptions(
			exprParts,
			syntax.syntax,
			syntax.name_to_syntaxAndExpr,
			ns,
			syntax.associativity,
			syntax.precedence.value_or(LONG_MIN),
			1,
			spansFailedToParseCache,
			logDepth+1
		);
		if (partialParseOptions.size() >= 1) {
			if (ret != nullptr) {
				if (!syntax.precedence.has_value()) continue;
				if (retPrecedence < syntax.precedence) continue;
				if (retPrecedence == syntax.precedence) throw SyntaxError("Ambiguous syntax"sv, FileRange::none());
			}
			retPrecedence = syntax.precedence.value_or(LONG_MAX);
			ret = syntax.expr->substitute(partialParseOptions[0]);
			#if EXPR_PARSER_LOG
			for (int i=0; i<logDepth; i++) std::print("|  ");
			std::print("Option: "); ret->print(); std::println();
			#endif
			break;
		}
	}
	
	#if EXPR_PARSER_LOG
	for (int i=0; i<logDepth; i++) std::print("|  ");
	#endif
	if (ret == nullptr) {
		if constexpr (EXPR_PARSER_LOG) std::println("fail");
		return std::nullopt;
	} else {
	#if EXPR_PARSER_LOG
		std::print("parseExpression done:  ");
		ret->print(); std::println();
		#endif
		return ret;
	}
}

[[nodiscard]] shared_ptr<const Expression> Parser::readExpression(
	Namespace& ns,
	char stopBeforeThisChar
) {
	vector<std::variant<shared_ptr<const Expression>, char, Space>> exprParts;
	
	FilePos exprStartPos = currentFilePos();
	
	while (true) {
		bool skippedWhitespace = false;
		{
			size_t prevSize = str.size();
			skipWhitespace();
			if (prevSize != str.size()) skippedWhitespace = true;
		}
		if (tryPeekChar(')') || (stopBeforeThisChar != 0x00 && tryPeekChar(stopBeforeThisChar)) || tryPeekChar(';')) break;
		
		auto startPos = currentFilePos();
		if (tryReadChar('(')) {
			auto expr = readExpression(ns, (char)0x00);
			skipWhitespace();
			readChar(')', "Expected ) to match ("sv);
			exprParts.push_back(std::move(expr));
		} else if (tryReadKeyword("forany"sv)) {
			Namespace ns2(&ns);
			vector<Id> foranyVars;
			do {
				skipWhitespace();
				auto foranyVarNameStart = currentFilePos();
				auto foranyVarName = readIdentifier("Expected forany variable name"sv);
				auto foranyVarId = ns2.make(foranyVarName, FileRange::startEnd(foranyVarNameStart, currentFilePos()));
				foranyVars.push_back(foranyVarId);
				skipWhitespace();
			} while (tryReadChar(','));
			readChar(':', "Expected : after forany variables"sv);
			auto subExpr = readExpression(ns2, stopBeforeThisChar);
			exprParts.push_back(std::make_shared<const Expression_ForAny>(FileRange::startEnd(startPos, currentFilePos()), std::move(foranyVars), std::move(subExpr)));
			break;
		} else if (str.size() != 0 && isOperatorChar(str[0])) {
			if (skippedWhitespace && exprParts.size() != 0 && std::holds_alternative<char>(exprParts.back())) {
				exprParts.push_back(Space{});
			}
			
			exprParts.emplace_back(readNonWhitespaceChar());
		} else {
			auto identStartPos = currentFilePos();
			auto maybeIdent = tryReadIdentifier();
			if (!maybeIdent.has_value()) break; //throw SyntaxError("Expected expression"sv, currentFilePos());
			auto maybeId = ns.find(maybeIdent.value());
			if (!maybeId.has_value()) {
				std::println("\n\n{}\n", maybeIdent.value());
				throw SyntaxError("Unknown identifier"sv, FileRange::startEnd(identStartPos, currentFilePos()));
			}
			exprParts.push_back(std::make_shared<Expression_Id>(FileRange::startEnd(identStartPos, currentFilePos()), maybeId.value()));
		}
	}
	
	std::set<ParseRegions> spansFailedToParseCache;
	auto maybeExpr = parseExpression(exprParts, ns, LONG_MIN, spansFailedToParseCache);
	if (!maybeExpr.has_value()) {
		//rewindTo(exprStartPos);
		//std::println("'{}'", str.substr(0, 100));
		throw SyntaxError("Expected expression"sv, FileRange::startEnd(exprStartPos, currentFilePos()));
	}
	return std::move(maybeExpr.value());
}
