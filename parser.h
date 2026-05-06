#pragma once

#include <string_view>
#include <string>
#include <optional>
#include <variant>
#include <vector>
#include <map>
#include <span>
#include <set>

#include "id.h"
#include "file_range.h"
#include "syntax_error.h"
#include "expression.h"
#include "syntax.h"
#include "namespace.h"
#include "proofsyntax.h"

using std::string_literals::operator""s;
using std::string_view_literals::operator""sv;
using std::string_view;
using std::optional;
using std::pair;
using std::variant;
using std::map;
using std::set;
using std::vector;
using std::span;

struct Statement;
struct Proof;
struct ProofSyntax;


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



/*struct TextSource {
	FileRange range;
	FileRange source;
};*/

struct Parser {
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
