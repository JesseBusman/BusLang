#include "parser.h"

// Skips whitespace, single-line comments and multi-line comments.
void Parser::skipWhitespace() {
	unsigned long skipped = 0;
	bool prevWasCR = false;
	while (skipped < str.size()) {
		if (skipped+1 < str.size() && str[skipped] == '/' && str[skipped+1] == '/') {
			col += 2;
			skipped += 2;
			while (skipped < str.size() && str[skipped] != '\r' && str[skipped] != '\n') {
				col++;
				skipped++;
			}
			if (skipped == str.size()) {
				break;
			}
		} else if (skipped+1 < str.size() && str[skipped] == '/' && str[skipped+1] == '*') {
			col += 2;
			skipped += 2;
			unsigned int multilineCommentNestingLevel = 1;
			while (skipped < str.size()) {
				if (skipped+1 < str.size() && str[skipped] == '/' && str[skipped+1] == '*') {
					col += 2;
					skipped += 2;
					multilineCommentNestingLevel++;
				}
				
				if (skipped+1 < str.size() && str[skipped] == '*' && str[skipped+1] == '/') {
					col += 2;
					skipped += 2;
					multilineCommentNestingLevel--;
					if (multilineCommentNestingLevel == 0) break;
				}
				
				if (str[skipped] == '\r') {
					prevWasCR = true;
					line++;
					col = 1;
				} else if (str[skipped] == '\n') {
					if (!prevWasCR) {
						line++;
						col = 1;
					}
					prevWasCR = false;
				} else {
					col++;
					prevWasCR = false;
				}
				skipped++;
			}
			if (skipped == str.size()) {
				break;
			}
		} else if (isWhitespaceChar(str[skipped])) {
			if (str[skipped] == '\r') {
				prevWasCR = true;
				line++;
				col = 1;
			} else if (str[skipped] == '\n') {
				if (!prevWasCR) {
					line++;
					col = 1;
				}
				prevWasCR = false;
			} else {
				col++;
				prevWasCR = false;
			}
			skipped++;
		} else {
			break;
		}
	}
	str = str.substr(skipped);
}
	
long Parser::readInteger(string_view errorMessage) {
	bool negative = false;
	long ret = 0;
	if (tryReadChar('-')) negative = true;
	if (str.size() == 0) throw SyntaxError(errorMessage, currentFilePos());
	int digits = 0;
	while (str[0] >= '0' && str[0] <= '9') {
		if (digits >= 18) throw SyntaxError("Integer literal is too large"sv, currentFilePos()); // TODO
		ret *= 10;
		ret += str[0] - '0';
		digits++;
		col++;
		str = str.substr(1);
	}
	if (digits == 0) throw SyntaxError(errorMessage, currentFilePos());
	return negative ? -ret : ret;
}

[[nodiscard]] optional<string_view> Parser::tryReadIdentifier() {
	if (str.size() == 0) return std::nullopt;
	if (!isIdentifierChar(str[0])) return std::nullopt;
	unsigned long length = 0;
	do {
		length++;
		if (length >= str.size()) break;
	} while (isIdentifierChar(str[length]));
	auto ret = str.substr(0, length);
	if (isKeyword(ret)) return std::nullopt;
	str = str.substr(length);
	col += length;
	return ret;
}

void Parser::skipParenEnclosedStuff() {
	auto parenOpenPos = currentFilePos();
	readChar('(', "Expected ("sv);
	unsigned int depth = 1;
	while (true) {
		skipWhitespace();
		if (str.size() == 0) throw SyntaxError("Expected ) to match ("sv, currentFilePos(), parenOpenPos);
		char c = str[0];
		col++;
		str = str.substr(1);
		if (c == '(') {
			depth++;
		} else if (c == ')') {
			depth--;
			if (depth == 0) {
				return;
			}
		}
	}
}

void Parser::skipToAndIncluding(char to) {
	while (true) {
		skipWhitespace();
		if (str.size() == 0) throw SyntaxError("Expected char "s + to, currentFilePos());
		if (str[0] == to) {
			col++;
			str = str.substr(1);
			return;
		} else if (str[0] == '(') {
			skipParenEnclosedStuff();
		} else {
			col++;
			str = str.substr(1);
		}
	}
}

[[nodiscard]] vector<std::variant<Id, char, Space, pair<Id, Id>>> Parser::readSyntaxPieces(Namespace& ns) {
	vector<std::variant<Id, char, Space, pair<Id, Id>>> ret;
	skipWhitespace();
	bool skippedWhitespace = false;
	while (!tryPeekChar(')')) {
		auto startPos = currentFilePos();
		if (auto paramName = tryReadIdentifier(); paramName.has_value()) {
			auto paramNameId = ns.make(paramName.value(), FileRange::startEnd(startPos, currentFilePos()));
			ret.emplace_back(paramNameId);
		} else if (str.size() != 0 && isOperatorChar(str[0])) {
			if (skippedWhitespace && ret.size() != 0 && std::holds_alternative<char>(ret.back())) ret.push_back(Space{});
			ret.emplace_back((char)str[0]);
			col++;
			str = str.substr(1);
		} else if (tryReadChar('(')) {
			skipWhitespace();
			auto paramNameStartPos = currentFilePos();
			auto paramName = readIdentifier(""sv);
			auto paramNameId = ns.make(paramName, FileRange::startEnd(paramNameStartPos, currentFilePos()));
			skipWhitespace();
			readChar(':', ""sv);
			skipWhitespace();
			auto syntaxNameStartPos = currentFilePos();
			auto syntaxName = readIdentifier(""sv);
			auto syntaxNameId = ns.find(syntaxName);
			if (!syntaxNameId.has_value()) throw SyntaxError("Unknown syntax name"sv, FileRange::startEnd(syntaxNameStartPos, currentFilePos()));
			skipWhitespace();
			readChar(')', ""sv);
			ret.emplace_back(pair{paramNameId, syntaxNameId.value()});
		} else {
			throw SyntaxError("Expected something else"sv, currentFilePos());
		}
		auto before = str.size();
		skipWhitespace();
		skippedWhitespace = before != str.size();
	}
	return ret;
}
