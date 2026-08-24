export module BusLang:Parser;

import std;
import :Expression;
import :Id;
import :Syntax;
import :Namespace;
import :SyntaxError;
import :Statement_Proof;

using std::string_literals::operator""s;
using std::string_view_literals::operator""sv;
using std::string_view;
using std::optional;
using std::pair;
using std::variant;
using std::map;
using std::set;
using std::vector;
using std::shared_ptr;
using std::span;

export {

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
		str == "require"sv ||
		str == "assume"sv ||
		str == "atom"sv ||
		str == "forany"sv;
}

}

/*struct TextSource {
	FileRange range;
	FileRange source;
};*/

export struct Parser {
	string_view fullStr;
	string_view str;
	unsigned int line = 1;
	unsigned int col = 1;
	//vector<TextSource> textSources;
	
	[[nodiscard]] constexpr string_view getStringViewFromTo(FilePos start, FilePos end) const noexcept {
		return string_view(fullStr.substr(start.index, end.index-start.index));
	}
	
	[[nodiscard]] constexpr FilePos currentFilePos() const noexcept {
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
	
	[[nodiscard]] constexpr bool isAtEnd() const noexcept { return str.size() == 0; }
	
	constexpr Parser(string_view _str/*, vector<TextSource>&& _textSources*/): fullStr(_str), str(_str)/*, textSources(std::move(_textSources))*/ { }
	
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
	
	[[nodiscard]] constexpr char readNonWhitespaceChar() {
		if (str.size() == 0) throw SyntaxError("Unexpected end of file"sv, currentFilePos());
		auto pos = currentFilePos();
		skipWhitespace();
		if (currentFilePos().index != pos.index) throw SyntaxError("Unexpected whitespace"sv, currentFilePos());
		char ret = str[0];
		str = str.substr(1);
		col++;
		return ret;
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
	
	[[nodiscard]] string_view peekIdentifierOrKeyword() const {
		unsigned int i = 0;
		while (i<str.size() && isIdentifierChar(str[i])) i++;
		if (i == 0) throw SyntaxError("Expected identifier or keyword"sv, currentFilePos());
		return str.substr(0, i);
	}
	[[nodiscard]] string_view readIdentifierOrKeyword() {
		unsigned int i = 0;
		while (i<str.size() && isIdentifierChar(str[i])) i++;
		if (i == 0) throw SyntaxError("Expected identifier or keyword"sv, currentFilePos());
		auto ret = str.substr(0, i);
		str = str.substr(i);
		col += i;
		return ret;
	}
	
	[[nodiscard]] vector<std::variant<Id, char, Space, pair<Id, Id>>> readSyntaxPieces(Namespace& ns);
	
	void skipParenEnclosedStuff(bool considerBracketsAndSquareBrackets);
	
	void skipToAndIncluding(char to, bool considerBracketsAndSquareBrackets);
	
	[[nodiscard]] string_view readUntilCharOrEnd(char c, bool considerBracketsAndSquareBrackets);
	
	[[nodiscard]] shared_ptr<const Proof> readStatementsAndMaybeOneProof(
		Namespace& ns,
		vector<shared_ptr<const Statement>>& ret,
		bool syntaxAssumePermitted
	);
	
	[[nodiscard]] ProofSyntax_Type readParseType();
	[[nodiscard]] ProofSyntax_ParsePiece readParsePiece();
	[[nodiscard]] ProofSyntax_OutputSegment readStuffInDoubleParens();
	[[nodiscard]] ProofSyntax_OutputSegment readStuffInDoubleParens_sub();
	[[nodiscard]] ProofSyntax_OutputSegment readSyntaxOutputSegment(unsigned int& depth);
	[[nodiscard]] shared_ptr<ProofSyntax> readProofSyntax(bool assume);
	
	/*
	[[nodiscard]] shared_ptr<const Proof> readProof_level0(
		Namespace& ns
	);
	
	[[nodiscard]] shared_ptr<const Proof> readProof(
		Namespace& ns
	);
	*/
	[[nodiscard]] shared_ptr<const Proof> readProof(
		Namespace& ns,
		char stopChar,
		bool syntaxAssumePermitted
	);
	
	[[nodiscard]] shared_ptr<const Expression> readExpression(
		Namespace& ns,
		char stopBeforeThisChar
	);
	[[nodiscard]] shared_ptr<const Expression> readExpression(
		Namespace& ns,
		bool _
	) = delete;
};

struct NothingHere {
	constexpr bool operator == (const NothingHere&) const noexcept { return true ; }
	constexpr bool operator != (const NothingHere&) const noexcept { return false; }
	constexpr bool operator <  (const NothingHere&) const noexcept { return false; }
	constexpr bool operator >  (const NothingHere&) const noexcept { return false; }
	constexpr bool operator <= (const NothingHere&) const noexcept { return true ; }
	constexpr bool operator >= (const NothingHere&) const noexcept { return true ; }
};

bool tryReadProofByCustomSyntax(
	Parser& p,
	Namespace& ns,
	span<const ProofSyntax_ParsePiece> parsePieces,
	const ProofSyntax& proofSyntax,
	map<string_view, ProofSyntax_MatchedValue>& outMatches,
	const set<std::variant<char, string_view, ProofSyntax_Type_Identifier, NothingHere>>& nextPiece,
	unsigned int callDepth
);
void getOutputSegmentStr(const ProofSyntax_OutputSegment& outputSegment, const ProofSyntax& proofSyntax, const map<string_view, ProofSyntax_MatchedValue>& pr, std::stringstream& out);

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

void Parser::skipParenEnclosedStuff(bool considerBracketsAndSquareBrackets) {
	auto parenOpenPos = currentFilePos();
	if (considerBracketsAndSquareBrackets) {
		vector<char> stack;
		if (tryReadChar('(')) stack.push_back(')');
		else if (tryReadChar('{')) stack.push_back('}');
		else if (tryReadChar('[')) stack.push_back(']');
		else throw SyntaxError("Expected ( or { or ["sv, currentFilePos());
		while (true) {
			skipWhitespace();
			if (str.size() == 0) throw SyntaxError("Expected ) or } or ] to match ( or { or ["sv, currentFilePos(), parenOpenPos);
			char c = readNonWhitespaceChar();
			if (c == '(') stack.push_back(')');
			else if (c == '{') stack.push_back('}');
			else if (c == '[') stack.push_back(']');
			else if (c == ')' || c == ']' || c == '}') {
				if (stack.back() == c) {
					stack.pop_back();
					if (stack.size() == 0) return;
				}
				else throw SyntaxError("Closing ] or ) or } did not match"sv, currentFilePos(), parenOpenPos);
			}
		}
	} else {
		readChar('(', "Expected ("sv);
		unsigned int depth = 1;
		while (true) {
			skipWhitespace();
			if (str.size() == 0) throw SyntaxError("Expected ) to match ("sv, currentFilePos(), parenOpenPos);
			char c = readNonWhitespaceChar();
			if (c == '(') depth++;
			else if (c == ')') {
				depth--;
				if (depth == 0) return;
			}
		}
	}
}

void Parser::skipToAndIncluding(char to, bool considerBracketsAndSquareBrackets) {
	while (true) {
		skipWhitespace();
		if (str.size() == 0 || str[0] == ')') throw SyntaxError("Expected char "s + to, currentFilePos());
		if (tryReadChar(to)) {
			return;
		} else if (str[0] == '(' || (considerBracketsAndSquareBrackets && (str[0] == '[' || str[0] == '{'))) {
			skipParenEnclosedStuff(considerBracketsAndSquareBrackets);
		} else {
			auto _ = readNonWhitespaceChar();
		}
	}
}

[[nodiscard]] string_view Parser::readUntilCharOrEnd(char c, bool considerBracketsAndSquareBrackets) {
	auto startPos = currentFilePos();
	if (considerBracketsAndSquareBrackets) {
		while (true) {
			skipWhitespace();
			if (str.size() == 0 || str[0] == ')' || str[0] == ']' || str[0] == '}' || str[0] == c) {
				return getStringViewFromTo(startPos, currentFilePos());
			} else if (str[0] == '(' || str[0] == '[' || str[0] == '{') {
				skipParenEnclosedStuff(true);
			} else {
				auto _ = readNonWhitespaceChar();
			}
		}
	} else {
		while (true) {
			skipWhitespace();
			if (str.size() == 0 || str[0] == ')' || str[0] == c) {
				return getStringViewFromTo(startPos, currentFilePos());
			} else if (str[0] == '(') {
				skipParenEnclosedStuff(false);
			} else {
				auto _ = readNonWhitespaceChar();
			}
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
			ret.emplace_back(readNonWhitespaceChar());
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
