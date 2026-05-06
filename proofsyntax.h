#pragma once

#include <vector>
#include <map>
#include <optional>
#include <string_view>
#include <variant>
#include <memory>

using std::map;
using std::vector;
using std::string_view;
using std::pair;
using std::optional;
using std::shared_ptr;







struct ProofSyntax_OutputSegment;
struct ProofSyntax_OutputSegment_Literal {
	std::variant<string_view, std::string> strv_or_str;
	constexpr string_view strv() const noexcept {
		if (auto strv=std::get_if<string_view>(&strv_or_str)) return *strv;
		else return std::get<std::string>(strv_or_str);
	}
};
struct ProofSyntax_OutputSegment_FunctionCall {
	string_view funcName;
	vector<shared_ptr<ProofSyntax_OutputSegment>> args;
};
struct ProofSyntax_OutputSegment_Variable {
	string_view varName;
};
struct ProofSyntax_OutputSegment_Sub {
	shared_ptr<ProofSyntax_OutputSegment> lhs;
	string_view rhs;
};

struct ProofSyntax_OutputSegment : std::variant<ProofSyntax_OutputSegment_Literal, ProofSyntax_OutputSegment_FunctionCall, ProofSyntax_OutputSegment_Variable, ProofSyntax_OutputSegment_Sub> {
	template<typename T>
	constexpr ProofSyntax_OutputSegment(T&& t):
		std::variant<ProofSyntax_OutputSegment_Literal, ProofSyntax_OutputSegment_FunctionCall, ProofSyntax_OutputSegment_Variable, ProofSyntax_OutputSegment_Sub>(std::forward<T>(t))
	{
	}
	constexpr ProofSyntax_OutputSegment(ProofSyntax_OutputSegment&&) = default;
	constexpr ProofSyntax_OutputSegment& operator = (ProofSyntax_OutputSegment&&) = default;
	~ProofSyntax_OutputSegment() = default;
};








struct ProofSyntax_MatchedValue;
struct ProofSyntax_MatchedValue_AnyStringView {
	string_view str;
};
struct ProofSyntax_MatchedValue_AnyString {
	std::string str;
};
struct ProofSyntax_MatchedValue_Identifier {
	string_view ident;
};
struct ProofSyntax_MatchedValue_SubSyntax {
	string_view subSyntaxName;
	unsigned long alternativeIndex;
	string_view matchedString;
	map<string_view, ProofSyntax_MatchedValue> matches;
};
struct ProofSyntax_MatchedValue : std::variant<ProofSyntax_MatchedValue_AnyStringView, ProofSyntax_MatchedValue_AnyString, ProofSyntax_MatchedValue_Identifier, ProofSyntax_MatchedValue_SubSyntax> {
	template<typename T>
	constexpr ProofSyntax_MatchedValue(T&& t):
		std::variant<ProofSyntax_MatchedValue_AnyStringView, ProofSyntax_MatchedValue_AnyString, ProofSyntax_MatchedValue_Identifier, ProofSyntax_MatchedValue_SubSyntax>(std::forward<T>(t))
	{
	}
	constexpr ProofSyntax_MatchedValue(ProofSyntax_MatchedValue&&) = default;
	constexpr ProofSyntax_MatchedValue& operator = (ProofSyntax_MatchedValue&&) = default;
	constexpr ProofSyntax_MatchedValue(const ProofSyntax_MatchedValue&) = default;
	constexpr ProofSyntax_MatchedValue& operator = (const ProofSyntax_MatchedValue&) = default;
	~ProofSyntax_MatchedValue() = default;
};





struct ProofSyntax_Type_Any { };
struct ProofSyntax_Type_Identifier {
	constexpr bool operator == (const ProofSyntax_Type_Identifier&) const noexcept { return true ; }
	constexpr bool operator != (const ProofSyntax_Type_Identifier&) const noexcept { return false; }
	constexpr bool operator <  (const ProofSyntax_Type_Identifier&) const noexcept { return false; }
	constexpr bool operator >  (const ProofSyntax_Type_Identifier&) const noexcept { return false; }
	constexpr bool operator <= (const ProofSyntax_Type_Identifier&) const noexcept { return true ; }
	constexpr bool operator >= (const ProofSyntax_Type_Identifier&) const noexcept { return true ; }
};
struct ProofSyntax_Type_Parentheses {
	constexpr bool operator == (const ProofSyntax_Type_Parentheses&) const noexcept { return true ; }
	constexpr bool operator != (const ProofSyntax_Type_Parentheses&) const noexcept { return false; }
	constexpr bool operator <  (const ProofSyntax_Type_Parentheses&) const noexcept { return false; }
	constexpr bool operator >  (const ProofSyntax_Type_Parentheses&) const noexcept { return false; }
	constexpr bool operator <= (const ProofSyntax_Type_Parentheses&) const noexcept { return true ; }
	constexpr bool operator >= (const ProofSyntax_Type_Parentheses&) const noexcept { return true ; }
};
struct ProofSyntax_Type_SubParse {
	string_view name;
};
struct ProofSyntax_Type_SubParseSpecificAlternative {
	string_view name;
	unsigned int alternativeIndex;
};
using ProofSyntax_Type = std::variant<ProofSyntax_Type_Any, ProofSyntax_Type_Identifier, ProofSyntax_Type_Parentheses, ProofSyntax_Type_SubParse, ProofSyntax_Type_SubParseSpecificAlternative>;






struct ProofSyntax_ParsePiece_Keyword {
	std::string_view str;
};
struct ProofSyntax_ParsePiece_Semicolon {
};
struct ProofSyntax_ParsePiece_OperatorChars {
	std::string_view str;
};
struct ProofSyntax_ParsePiece_Typed {
	std::string_view name;
	ProofSyntax_Type type;
};
using ProofSyntax_ParsePiece = std::variant<ProofSyntax_ParsePiece_Keyword, ProofSyntax_ParsePiece_Semicolon, ProofSyntax_ParsePiece_OperatorChars, ProofSyntax_ParsePiece_Typed>;







struct ProofSyntax_SubParseDefinition {
	vector<vector<ProofSyntax_ParsePiece>> alternatives;
};






struct ProofSyntax_Function {
	string_view funcName;
	vector<pair<string_view, ProofSyntax_Type>> paramNamesAndTypes;
	vector<ProofSyntax_OutputSegment> output;
	constexpr ProofSyntax_Function(string_view _funcName, vector<pair<string_view, ProofSyntax_Type>>&& _paramNamesAndTypes, vector<ProofSyntax_OutputSegment>&& _output):
		funcName(_funcName), paramNamesAndTypes(std::move(_paramNamesAndTypes)), output(std::move(_output)) { }
	constexpr ProofSyntax_Function(ProofSyntax_Function&&) = default;
	constexpr ProofSyntax_Function& operator = (ProofSyntax_Function&&) = default;
	~ProofSyntax_Function() = default;
};






struct ProofSyntax {
	map<std::string_view, ProofSyntax_SubParseDefinition> subParseDefinitions;
	vector<ProofSyntax_Function> functions;
	vector<ProofSyntax_ParsePiece> mainParseDefinition;
	vector<ProofSyntax_OutputSegment> mainOutput;
	bool isAssumed;
};
