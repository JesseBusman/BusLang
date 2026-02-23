#pragma once

#include <string_view>
#include <string>
#include <optional>
#include <variant>
#include <vector>
#include <map>

#include "id.h"
#include "file_range.h"
#include "syntax_error.h"
#include "expression.h"
#include "syntax.h"
#include "namespace.h"

using std::string_literals::operator""s;
using std::string_view_literals::operator""sv;
using std::string_view;
using std::optional;
using std::pair;
using std::variant;
using std::map;
using std::vector;

struct Statement;
struct Proof;


[[nodiscard]] constexpr bool isKeywordChar(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

[[nodiscard]] constexpr bool isOperatorChar(char c) {
	return (c >= '!' && c <= '\'') || (c >= '*' && c <= '/') || c == ':' || (c >= '<' && c <= '@') || (c >= '[' && c <= '^') || c == '`' || (c >= '{' && c <= '~');
}

[[nodiscard]] constexpr bool isWhitespaceChar(char c) {
	return c == ' ' || c == '\r' || c == '\n' || c == '\t';
}

[[nodiscard]] constexpr bool isIdentifierChar(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

[[nodiscard]] constexpr bool isKeyword(string_view str) {
	return
		str == "scope"sv ||
		str == "proves"sv ||
		str == "by"sv ||
		str == "in"sv ||
		str == "substitute"sv ||
		str == "unwrap"sv ||
		str == "wrap"sv ||
		str == "require"sv ||
		str == "assume"sv ||
		str == "atom"sv ||
		str == "define"sv ||
		str == "forany"sv;
}





struct Parser {
	string_view fullStr;
	string_view str;
	unsigned int line = 1;
	unsigned int col = 1;
	
	constexpr FilePos currentFilePos() const noexcept {
		return {
			.index = (unsigned int)fullStr.size() - (unsigned int)str.size(),
			.line = line,
			.col = col,
		};
	}
	
	void rewindTo(FilePos pos) noexcept {
		str = fullStr.substr(pos.index);
		line = pos.line;
		col = pos.col;
	}
	
	constexpr bool isAtEnd() const noexcept { return str.size() == 0; }
	
	constexpr Parser(string_view _str): fullStr(_str), str(_str) { }
	
	// Skips whitespace, single-line comments and multi-line comments.
	void skipWhitespace();
	
	[[nodiscard]] constexpr bool areAtEnd() const {
		return str.size() == 0;
	}
	
	[[nodiscard]] constexpr bool tryPeekChar(char c) const {
		return str.size() != 0 && str[0] == c;
	}
	
	[[nodiscard]] constexpr bool tryReadChar(char c) {
		if (str.size() != 0 && str[0] == c) {
			str = str.substr(1);
			col++;
			return true;
		} else {
			return false;
		}
	}
	
	constexpr void readChar(char c, string_view errorMessage) {
		auto maybe = tryReadChar(c);
		if (!maybe) throw SyntaxError(errorMessage, currentFilePos());
	}
	
	[[nodiscard]] constexpr bool tryReadChars(string_view chars) {
		if (chars.size() > str.size()) return false;
		if (str.substr(0, chars.size()) == chars) {
			str = str.substr(chars.size());
			col += chars.size();
			return true;
		}
		return false;
	}
	
	[[nodiscard]] constexpr bool tryReadKeyword(string_view keyword) {
		if (keyword.size() > str.size()) return false;
		if (str.substr(0, keyword.size()) != keyword) return false;
		if (keyword.size() == str.size() || !isIdentifierChar(str[keyword.size()])) {
			str = str.substr(keyword.size());
			col += keyword.size();
			return true;
		}
		return false;
	}
	
	void readKeyword(string_view keyword, string_view errorMessage) {
		auto maybe = tryReadKeyword(keyword);
		if (!maybe) throw SyntaxError(errorMessage, currentFilePos());
	}
	
	long readInteger(string_view errorMessage);
	
	[[nodiscard]] optional<string_view> tryReadIdentifier();
	
	[[nodiscard]] inline string_view readIdentifier(string_view errorMessage) {
		auto maybe = tryReadIdentifier();
		if (maybe.has_value()) { return maybe.value(); }
		else { throw SyntaxError(errorMessage, currentFilePos()); }
	}
	
	[[nodiscard]] vector<std::variant<Id, char, Space, pair<Id, Id>>> readSyntaxPieces(Namespace& ns);
	
	void skipParenEnclosedStuff();
	
	void skipToAndIncluding(char to);
	
	[[nodiscard]] pair<vector<shared_ptr<const Statement>>, shared_ptr<const Proof>> readStatementsAndMaybeOneProof(
		Namespace& ns
	);
	
	
	
	[[nodiscard]] shared_ptr<const Proof> readProof_level0(
		Namespace& ns
	);
	
	[[nodiscard]] shared_ptr<const Proof> readProof(
		Namespace& ns
	);
	
	
	
	
	
	
	[[nodiscard]] shared_ptr<const Expression> readExpression(
		Namespace& ns,
		bool stopBeforeComma
	);
	
	
	
	/*
	[[nodiscard]] shared_ptr<const Expression> tryReadExpression_level0(
		Namespace& ns
	) {
		string_view startStr = str;
		unsigned int startLine = line;
		unsigned int startCol = col;
		
		FilePos startFilePos = currentFilePos();
		
		skipWhitespace();
		FilePos startFilePos2 = currentFilePos();
		if (tryReadChar('(')) {
			auto openingBracketLine = line;
			auto openingBracketCol = col-1;
			auto openingBracketStrLenAfter = str.size();
			auto ret = readExpression(ns);
			skipWhitespace();
			if (!tryReadChar(')')) {
				throw SyntaxError("( did not match with any )"s, startFilePos, currentFilePos());
			}
			//readChar(')');
			return ret;
		} else if (auto maybeIdent = tryReadIdentifier(); maybeIdent.has_value()) {
			if (auto it=ns.find(maybeIdent.value()); it.has_value()) {
				return std::make_shared<Expression_Id>(FileRange::startEnd(startFilePos2, currentFilePos()), it.value());
			} else {
				str = startStr;
				line = startLine;
				col = startCol;
				throw SyntaxError("Unknown identifier"s, startFilePos2);
			}
		} else {
			return nullptr;
		}
	}
	
	[[nodiscard]] shared_ptr<const Expression> readExpression_level1(
		Namespace& ns
	) {
		FilePos startFilePos = currentFilePos();
		auto ret = tryReadExpression_level0(ns);
		if (ret == nullptr) {
			throw SyntaxError("Expected expression", currentFilePos());
		}
		
		while (true) {
			auto expr2 = tryReadExpression_level0(ns);
			if (expr2 == nullptr) break;
			ret = std::make_shared<Expression_Apply>(FileRange::startEnd(startFilePos, currentFilePos()), std::move(ret), std::move(expr2));
		}
		
		return ret;
	}
	
	[[nodiscard]] shared_ptr<const Expression> readExpression_level2(
		Namespace& ns
	) {
		vector<shared_ptr<const Expression>> exprs;
		exprs.emplace_back(readExpression_level1(ns));
		skipWhitespace();
		while (tryReadChars("->"sv)) {
			skipWhitespace();
			exprs.emplace_back(readExpression_level1(ns));
			skipWhitespace();
		}
		
		shared_ptr<const Expression> ret = std::move(exprs.back());
		for (int i=exprs.size()-2; i>=0; i--) {
			ret = std::make_shared<Expression_Apply>(
				FileRange::span(exprs[i]->fileRange, ret->fileRange),
				std::make_shared<Expression_Apply>(
					exprs[i]->fileRange,
					auto{ATOM_IMPLIES},
					std::move(exprs[i])
				),
				std::move(ret)
			);
		}
		return ret;
	}
	
	[[nodiscard]] shared_ptr<const Expression> readExpression(
		Namespace& ns
	) {
		shared_ptr<const Expression> ret;
		skipWhitespace();
		
		FilePos startFilePos = currentFilePos();
		
		if (tryReadKeyword("forany"sv)) {
			Namespace ns2(&ns);
			vector<Id> varIds;
			do {
				skipWhitespace();
				auto varNameStart = currentFilePos();
				auto varName = tryReadIdentifier();
				if (!varName.has_value()) {
					throw SyntaxError("Expected identifier (variable name) or : in forany variable list", currentFilePos());
				}
				varIds.push_back(ns2.make(varName.value(), FileRange::startEnd(varNameStart, currentFilePos())));
				skipWhitespace();
			} while (tryReadChar(','));
			readChar(':', "Expected : after forany variable list"sv);
			skipWhitespace();
			
			auto subExpr = readExpression(ns2);
			
			ret = std::make_shared<Expression_ForAny>(FileRange::startEnd(startFilePos, currentFilePos()), std::move(varIds), std::move(subExpr));
		} else {
			ret = readExpression_level2(ns);
		}
		
		return ret;
	}*/
	
	
	
	
};
