module BusLang;

import std;
import :Parser;
import :Id;
import :Expression;
import :Namespace;

/*
struct ParseResult {
	std::map<string_view, std::variant<string_view, std::string, shared_ptr<ParseResult>>> results;
};
*/

static set<std::variant<char, string_view, ProofSyntax_Type_Identifier, NothingHere>> getFirstOperatorCharOrKeyword(const ProofSyntax_ParsePiece& parsePiece, const ProofSyntax& proofSyntax) {
	if (auto k=std::get_if<ProofSyntax_ParsePiece_Keyword>(&parsePiece)) {
		return {k->str};
	} else if (std::holds_alternative<ProofSyntax_ParsePiece_Semicolon>(parsePiece)) {
		return {';'};
	} else if (auto o=std::get_if<ProofSyntax_ParsePiece_OperatorChars>(&parsePiece)) {
		return {o->str[0]};
	} else if (std::holds_alternative<ProofSyntax_ParsePiece_Typed>(parsePiece)) {
		auto& t = std::get<ProofSyntax_ParsePiece_Typed>(parsePiece);
		if (std::holds_alternative<ProofSyntax_Type_SubParse>(t.type)) {
			auto& sub = std::get<ProofSyntax_Type_SubParse>(t.type);
			auto it = proofSyntax.subParseDefinitions.find(sub.name);
			if (it == proofSyntax.subParseDefinitions.end()) { std::println("\n{}\n", sub.name); throw "sub parse does not exist"; }
			set<std::variant<char, string_view, ProofSyntax_Type_Identifier, NothingHere>> ret;
			for (auto& alternative : it->second.alternatives) {
				if (alternative.size() == 0) {
					ret.insert(NothingHere{});
				} else {
					ret.insert_range(getFirstOperatorCharOrKeyword(alternative[0], proofSyntax));
				}
			}
			return ret;
		} else if (std::holds_alternative<ProofSyntax_Type_SubParseSpecificAlternative>(t.type)) {
			auto& sub = std::get<ProofSyntax_Type_SubParseSpecificAlternative>(t.type);
			auto it = proofSyntax.subParseDefinitions.find(sub.name);
			if (it == proofSyntax.subParseDefinitions.end()) { std::println("\n{}\n", sub.name); throw "sub parse does not exist"; }
			if (sub.alternativeIndex >= it->second.alternatives.size()) { std::println("\n{}.{}\n", sub.name, sub.alternativeIndex); throw "alternative index out of range"; }
			if (it->second.alternatives[sub.alternativeIndex].size() == 0) return {NothingHere{}};
			else return getFirstOperatorCharOrKeyword(it->second.alternatives[sub.alternativeIndex][0], proofSyntax);
		} else if (std::holds_alternative<ProofSyntax_Type_Identifier>(t.type)) {
			return {ProofSyntax_Type_Identifier{}};
		} else if (std::holds_alternative<ProofSyntax_Type_Parentheses>(t.type)) {
			return {'('};
		} else if (std::holds_alternative<ProofSyntax_Type_Any>(t.type)) {
			//std::println("\nbefore {}\n", t.name);
			throw "Any followed by any in syntax";
		} else {
			throw 981981215;
		}
	} else {
		throw 9123213;
	}
}

bool tryReadProofByCustomSyntax(
	Parser& p,
	Namespace& ns,
	span<const ProofSyntax_ParsePiece> parsePieces,
	const ProofSyntax& proofSyntax,
	map<string_view, ProofSyntax_MatchedValue>& outMatches,
	const set<std::variant<char, string_view, ProofSyntax_Type_Identifier, NothingHere>>& stopBefore,
	unsigned int callDepth
) {
	if (parsePieces.size() == 0) return true;
	
	const auto indent = std::string(callDepth*2, ' ');
	
	/*std::println("{}tryReadProofByCustomSyntax(): ####", indent);
	std::println("{}tryReadProofByCustomSyntax(): str='{}'", indent, p.str.substr(0, 20));
	std::print("{}tryReadProofByCustomSyntax(): parsePieces=[", indent);
	for (const auto& parsePiece : parsePieces) {
		if (std::holds_alternative<ProofSyntax_ParsePiece_Semicolon>(parsePiece)) {
			std::print(", ';'");
		} else if (auto k=std::get_if<ProofSyntax_ParsePiece_Keyword>(&parsePiece)) {
			std::print(", '{}'", k->str);
		} else if (auto opc=std::get_if<ProofSyntax_ParsePiece_OperatorChars>(&parsePiece)) {
			std::print(", '{}'", opc->str);
		} else if (auto t=std::get_if<ProofSyntax_ParsePiece_Typed>(&parsePiece)) {
			std::print(", {}: ", t->name);
			if (std::holds_alternative<ProofSyntax_Type_Identifier>(t->type)) {
				std::print("IDENT");
			} else if (std::holds_alternative<ProofSyntax_Type_Parentheses>(t->type)) {
				std::print("PAREN");
			} else if (std::holds_alternative<ProofSyntax_Type_Any>(t->type)) {
				std::print("ANY");
			} else if (std::holds_alternative<ProofSyntax_Type_SubParse>(t->type)) {
				std::print("{}", std::get<ProofSyntax_Type_SubParse>(t->type).name);
			} else if (std::holds_alternative<ProofSyntax_Type_SubParseSpecificAlternative>(t->type)) {
				std::print("{}.{}", std::get<ProofSyntax_Type_SubParseSpecificAlternative>(t->type).name, std::get<ProofSyntax_Type_SubParseSpecificAlternative>(t->type).alternativeIndex);
			} else throw 339814;
		} else throw 9827323;
	}
	std::println("]");*/
	
	auto startPos = p.currentFilePos();
	
	//std::println("{}tryReadProofByCustomSyntax(): str='{}'", indent, p.str.substr(0, 20));
	const auto& parsePiece = parsePieces[0];
	
	if (auto keyword=std::get_if<ProofSyntax_ParsePiece_Keyword>(&parsePiece)) {
		//std::println("{}tryReadProofByCustomSyntax(): ProofSyntax_ParsePiece_Keyword: {}", indent, keyword->str);
		p.skipWhitespace();
		if (!p.tryReadKeyword(keyword->str) || !tryReadProofByCustomSyntax(p, ns, parsePieces.subspan(1), proofSyntax, outMatches, stopBefore, callDepth)) goto nope;
		//std::println("{}tryReadProofByCustomSyntax(): ProofSyntax_ParsePiece_Keyword: yup", indent);
		return true;
	} else if (std::holds_alternative<ProofSyntax_ParsePiece_Semicolon>(parsePiece)) {
		//std::println("{}tryReadProofByCustomSyntax(): ProofSyntax_ParsePiece_Semicolon: ;", indent);
		if (!p.tryReadChar(';') || !tryReadProofByCustomSyntax(p, ns, parsePieces.subspan(1), proofSyntax, outMatches, stopBefore, callDepth)) goto nope;
		//std::println("{}tryReadProofByCustomSyntax(): ProofSyntax_ParsePiece_Semicolon: yup", indent);
		return true;
	} else if (auto ops=std::get_if<ProofSyntax_ParsePiece_OperatorChars>(&parsePiece)) {
		//std::println("{}tryReadProofByCustomSyntax(): ProofSyntax_ParsePiece_OperatorChars: {}", indent, ops->str);
		p.skipWhitespace();
		if (!p.tryReadChars(ops->str) || !tryReadProofByCustomSyntax(p, ns, parsePieces.subspan(1), proofSyntax, outMatches, stopBefore, callDepth)) goto nope;
		//std::println("{}tryReadProofByCustomSyntax(): ProofSyntax_ParsePiece_OperatorChars: yup", indent);
		return true;
	} else if (std::holds_alternative<ProofSyntax_ParsePiece_Typed>(parsePiece)) {
		auto& t = std::get<ProofSyntax_ParsePiece_Typed>(parsePiece);
		if (outMatches.contains(t.name)) throw SyntaxError("Double match"sv, p.currentFilePos());
		if (std::holds_alternative<ProofSyntax_Type_Parentheses>(t.type)) {
			//std::println("{}tryReadProofByCustomSyntax(): ParsePiece_Typed: {}:PAREN", indent, t.name);
			p.skipWhitespace();
			if (!p.tryPeekChar('(')) goto nope;
			const auto start = p.currentFilePos();
			p.skipParenEnclosedStuff(true);
			auto sv = p.getStringViewFromTo(start, p.currentFilePos());
			if (!tryReadProofByCustomSyntax(p, ns, parsePieces.subspan(1), proofSyntax, outMatches, stopBefore, callDepth)) goto nope;
			outMatches.insert({t.name, ProofSyntax_MatchedValue_AnyStringView(sv)});
			//std::println("{}tryReadProofByCustomSyntax(): ParsePiece_Typed: yup '{}'", indent, sv);
			return true;
		} else if (std::holds_alternative<ProofSyntax_Type_Identifier>(t.type)) {
			//std::println("{}tryReadProofByCustomSyntax(): ParsePiece_Typed: {}:IDENT", indent, t.name);
			p.skipWhitespace();
			auto maybeIdent = p.tryReadIdentifier();
			if (!maybeIdent.has_value() || !tryReadProofByCustomSyntax(p, ns, parsePieces.subspan(1), proofSyntax, outMatches, stopBefore, callDepth)) goto nope;
			outMatches.insert({t.name, ProofSyntax_MatchedValue_Identifier(maybeIdent.value())});
			//std::println("{}tryReadProofByCustomSyntax(): ParsePiece_Typed: yup '{}'", indent, maybeIdent.value());
			return true;
		} else if (std::holds_alternative<ProofSyntax_Type_Any>(t.type)) {
			//std::println("{}tryReadProofByCustomSyntax(): ParsePiece_Typed: Any", indent);
			//const auto nextOptions = (i+1 < parsePieces.size()) ? getFirstOperatorCharOrKeyword(parsePieces[i+1], proofSyntax) : stopBefore;
			const auto nextOptions = parsePieces.size() >= 2 ? getFirstOperatorCharOrKeyword(parsePieces[1], proofSyntax) : stopBefore;
			
			/*if (maybeNextParsePiece == nullptr) {
				std::println("{}tryReadProofByCustomSyntax(): ParsePiece_Typed: Any: no next piece.", indent);
				auto str = p.readUntilCharOrEnd(')');
				outMatches.insert({t.name, ProofSyntax_MatchedValue_AnyStringView(str)});
			} else {*/
				//auto nextOptions = getFirstOperatorCharOrKeyword(*maybeNextParsePiece, proofSyntax);
				//std::println("{}tryReadProofByCustomSyntax(): ParsePiece_Typed: Any: {} next pieces:", indent, nextOptions.size());
				for (auto& opt : nextOptions) {
					if (std::holds_alternative<char>(opt)) std::println("    '{}'", std::get<char>(opt));
					else if (std::holds_alternative<string_view>(opt)) std::println("    \"{}\"", std::get<string_view>(opt));
					else if (std::holds_alternative<NothingHere>(opt)) std::println("    nothing");
					else if (std::holds_alternative<ProofSyntax_Type_Identifier>(opt)) std::println("    identifier");
					else throw 18781724;
				}
				auto startPos_ = p.currentFilePos();
				while (true) {
					p.skipWhitespace();
					if (p.areAtEnd()) {
						if (nextOptions.contains(NothingHere{})) break;
						else goto nope;
					}
					if (isOperatorChar(p.str[0]) || p.str[0] == ';') {
						//std::println("   op char {}", p.str[0]);
						if (nextOptions.contains(p.str[0])) break;
						auto _ = p.readNonWhitespaceChar();
					} else if (isKeywordChar(p.str[0]) || isIdentifierChar(p.str[0])) {
						auto ident = p.peekIdentifierOrKeyword();
						if (nextOptions.contains(ProofSyntax_Type_Identifier{})) break;
						if (nextOptions.contains(ident)) break;
						auto _ = p.readIdentifierOrKeyword();
					} else if (p.str[0] == '(' || p.str[0] == '{' || p.str[0] == '[') {
						p.skipParenEnclosedStuff(true);
					} else {
						throw 3684736478;
					}
				}
				string_view str = p.getStringViewFromTo(startPos_, p.currentFilePos());
				if (!tryReadProofByCustomSyntax(p, ns, parsePieces.subspan(1), proofSyntax, outMatches, stopBefore, callDepth)) goto nope;
				outMatches.insert({t.name, ProofSyntax_MatchedValue_AnyStringView(str)});
				//std::println("{} matched \"{}\"", t.name, str);
				return true;
			//}
		} else if (std::holds_alternative<ProofSyntax_Type_SubParse>(t.type)) {
			auto& subParse = std::get<ProofSyntax_Type_SubParse>(t.type);
			
			//const auto nextOptions = (i+1 < parsePieces.size()) ? getFirstOperatorCharOrKeyword(parsePieces[i+1], proofSyntax) : stopBefore;
			const auto nextOptions = parsePieces.size() >= 2 ? getFirstOperatorCharOrKeyword(parsePieces[1], proofSyntax) : stopBefore;
			//std::println("{}tryReadProofByCustomSyntax(): ParsePiece_Typed: SubParse: {}", indent, subParse.name);
			auto it = proofSyntax.subParseDefinitions.find(subParse.name);
			if (it == proofSyntax.subParseDefinitions.end()) { std::println("\n\n{}\n\n", subParse.name); throw SyntaxError("Unknown sub parse definition"sv, p.currentFilePos()); }
			//for (auto& alternative : it->second.alternatives) {
			for (unsigned int i=0; i<it->second.alternatives.size(); i++) {
				map<string_view, ProofSyntax_MatchedValue> subParseResult;
				const char* startChar = &p.str[0];
				if (tryReadProofByCustomSyntax(p, ns, it->second.alternatives[i], proofSyntax, subParseResult, nextOptions, callDepth+1)) {
					const char* endChar = &p.str[0];
					//std::println("{}tryReadProofByCustomSyntax(): yup", indent);
					
					if (tryReadProofByCustomSyntax(p, ns, parsePieces.subspan(1), proofSyntax, outMatches, stopBefore, callDepth)) {
						outMatches.insert({t.name, ProofSyntax_MatchedValue_SubSyntax(subParse.name, i, string_view(startChar, endChar), std::move(subParseResult))});
						return true;
					} else {
						p.rewindTo(startPos);
					}
				}
			}
			goto nope;
		} else if (std::holds_alternative<ProofSyntax_Type_SubParseSpecificAlternative>(t.type)) {
			auto& subParse = std::get<ProofSyntax_Type_SubParseSpecificAlternative>(t.type);
			//const auto nextOptions = (i+1 < parsePieces.size()) ? getFirstOperatorCharOrKeyword(parsePieces[i+1], proofSyntax) : stopBefore;
			const auto nextOptions = parsePieces.size() >= 2 ? getFirstOperatorCharOrKeyword(parsePieces[1], proofSyntax) : stopBefore;
			//std::println("{}tryReadProofByCustomSyntax(): ParsePiece_Typed: SubParseSpecificAlternative: {}.{}", indent, subParse.name, subParse.alternativeIndex);
			auto it = proofSyntax.subParseDefinitions.find(subParse.name);
			if (it == proofSyntax.subParseDefinitions.end()) { std::println("\n\n{}\n\n", subParse.name); throw SyntaxError("Unknown sub parse definition"sv, p.currentFilePos()); }
			if (subParse.alternativeIndex >= it->second.alternatives.size()) { std::println("\n\n{} {}\n\n", subParse.name, subParse.alternativeIndex); throw SyntaxError("Alternative index is out of bounds"sv, p.currentFilePos()); }
			auto& alternative = it->second.alternatives[subParse.alternativeIndex];
			//auto startPos = p.currentFilePos();
			map<string_view, ProofSyntax_MatchedValue> subParseResult;
			const char* startChar = &p.str[0];
			if (tryReadProofByCustomSyntax(p, ns, alternative, proofSyntax, subParseResult, nextOptions, callDepth+1)) {
				const char* endChar = &p.str[0];
				if (!tryReadProofByCustomSyntax(p, ns, parsePieces.subspan(1), proofSyntax, outMatches, stopBefore, callDepth)) goto nope;
				outMatches.insert({t.name, ProofSyntax_MatchedValue_SubSyntax(subParse.name, subParse.alternativeIndex, string_view(startChar, endChar), std::move(subParseResult))});
				//std::println("{}tryReadProofByCustomSyntax(): yup", indent);
				return true;
			} else {
				goto nope;
			}
		} else {
			throw 918233213;
		}
	} else {
		throw 928348923;
	}
	
	nope:;
	//std::println("{}tryReadProofByCustomSyntax(): nope", indent);
	p.rewindTo(startPos);
	return false;
}





static constexpr bool isOfType(const ProofSyntax_MatchedValue& value, const ProofSyntax_Type& type) {
	if (std::holds_alternative<ProofSyntax_Type_Any>(type)) return true; // TODO: Make a separate type for 'any string' and 'any value' (any value could also match a ProofSyntax_MatchedValue_SubSyntax, any string could not)
	if (std::holds_alternative<ProofSyntax_MatchedValue_AnyStringView>(value)) {
		return std::holds_alternative<ProofSyntax_Type_Any>(type);
	} else if (std::holds_alternative<ProofSyntax_MatchedValue_AnyString>(value)) {
		return std::holds_alternative<ProofSyntax_Type_Any>(type);
	} else if (std::holds_alternative<ProofSyntax_MatchedValue_Identifier>(value)) {
		return std::holds_alternative<ProofSyntax_Type_Any>(type);
	} else if (std::holds_alternative<ProofSyntax_MatchedValue_SubSyntax>(value)) {
		auto& v = std::get<ProofSyntax_MatchedValue_SubSyntax>(value);
		if (std::holds_alternative<ProofSyntax_Type_SubParse>(type)) {
			return std::get<ProofSyntax_Type_SubParse>(type).name == v.subSyntaxName;
		} else if (std::holds_alternative<ProofSyntax_Type_SubParseSpecificAlternative>(type)) {
			auto& t = std::get<ProofSyntax_Type_SubParseSpecificAlternative>(type);
			//std::println("isOfType({}.{}, {}.{})", v.subSyntaxName, v.alternativeIndex, t.name, t.alternativeIndex);
			return t.name == v.subSyntaxName && t.alternativeIndex == v.alternativeIndex;
		} else {
			return false;
		}
	} else {
		throw 95982353;
	}
}


static void getOutputSegmentStr(
	const ProofSyntax_OutputSegment& outputSegment,
	const ProofSyntax& proofSyntax,
	const map<string_view, ProofSyntax_MatchedValue>& pr,
	std::stringstream& out,
	unsigned int callDepth
);
static ProofSyntax_MatchedValue getOutputSegment(
	const ProofSyntax_OutputSegment& outputSegment,
	const ProofSyntax& proofSyntax,
	const map<string_view, ProofSyntax_MatchedValue>& pr,
	unsigned int callDepth
) {
	if (std::holds_alternative<ProofSyntax_OutputSegment_Literal>(outputSegment)) {
		auto str = std::get<ProofSyntax_OutputSegment_Literal>(outputSegment).strv();
		for (char c : str) {
			if (!isIdentifierChar(c)) return ProofSyntax_MatchedValue_AnyStringView(str);
		}
		return ProofSyntax_MatchedValue_Identifier(str);
	} else if (std::holds_alternative<ProofSyntax_OutputSegment_FunctionCall>(outputSegment)) {
		auto& funcCall = std::get<ProofSyntax_OutputSegment_FunctionCall>(outputSegment);
		
		if (funcCall.funcName == "replace_ident"sv) {
			if (funcCall.args.size() != 3) throw SyntaxError("replace_ident must have 3 arguments"sv, FilePos{});
			std::stringstream from, to, subject;
			getOutputSegmentStr(*funcCall.args[0], proofSyntax, pr, from);
			getOutputSegmentStr(*funcCall.args[1], proofSyntax, pr, to);
			getOutputSegmentStr(*funcCall.args[2], proofSyntax, pr, subject);
			std::string from_    = std::move(from   ).str();
			std::string to_      = std::move(to     ).str();
			std::string subject_ = std::move(subject).str();
			std::stringstream ret;
			Parser p(subject_);
			while (true) {
				if (p.areAtEnd()) break;
				auto posBeforeSkipWhitespace = p.currentFilePos();
				p.skipWhitespace();
				if (p.currentFilePos().index != posBeforeSkipWhitespace.index) {
					ret << ' ';
					if (p.areAtEnd()) break;
				}
				if (auto ident = p.tryReadIdentifier()) {
					if (ident.value() == from_) ret << to_;
					else ret << ident.value();
				} else {
					ret << p.readNonWhitespaceChar();
				}
			}
			return ProofSyntax_MatchedValue_AnyString(std::move(ret).str());
		}
		
		vector<ProofSyntax_MatchedValue> args;
		args.reserve(funcCall.args.size());
		for (auto& arg : funcCall.args) {
			args.push_back(getOutputSegment(*arg, proofSyntax, pr, callDepth+1));
		}
		//std::println("{}getOutputSegment(): Function call on {} with {} args", std::string(callDepth*2, ' '), funcCall.funcName, funcCall.args.size());
		
		if (funcCall.funcName == "error"sv) {
			if (funcCall.args.size() >= 1) {
				if (auto* lit=std::get_if<ProofSyntax_OutputSegment_Literal>(funcCall.args[0].get())) {
					std::visit([](auto&& str){throw SyntaxError(str, FilePos{});}, lit->strv_or_str);
				}
			}
			throw SyntaxError("error"sv, FilePos{});
		}
		
		if (funcCall.funcName == "concat"sv) {
			if (args.size() == 0) { return ProofSyntax_MatchedValue_AnyStringView(""sv); }
			else if (args.size() == 1) {
				if (auto ident=std::get_if<ProofSyntax_MatchedValue_Identifier>(&args[0])) return ProofSyntax_MatchedValue_AnyStringView(ident->ident);
				else if (auto s=std::get_if<ProofSyntax_MatchedValue_AnyString>(&args[0])) return ProofSyntax_MatchedValue_AnyString(std::move(s->str));
				else if (auto sv=std::get_if<ProofSyntax_MatchedValue_AnyStringView>(&args[0])) return ProofSyntax_MatchedValue_AnyStringView(sv->str);
				else if (auto sv=std::get_if<ProofSyntax_MatchedValue_SubSyntax>(&args[0])) return ProofSyntax_MatchedValue_AnyStringView(sv->matchedString);
				else throw 1984983;
			} else {
				std::stringstream ret;
				for (auto& arg : args) {
					if (auto ident=std::get_if<ProofSyntax_MatchedValue_Identifier>(&arg)) ret << ident->ident;
					else if (auto s=std::get_if<ProofSyntax_MatchedValue_AnyString>(&arg)) ret << s->str;
					else if (auto sv=std::get_if<ProofSyntax_MatchedValue_AnyStringView>(&arg)) ret << sv->str;
					else if (auto sv=std::get_if<ProofSyntax_MatchedValue_SubSyntax>(&arg)) ret << sv->matchedString;
					else throw 1984983;
				}
				return ProofSyntax_MatchedValue_AnyString(std::move(ret).str());
			}
		}
		
		bool foundButWrongArgCount = false;
		bool foundButWrongArgTypes = false;
		for (auto& func : proofSyntax.functions) { // TODO: make functions a map?
			if (func.funcName == funcCall.funcName) {
				if (args.size() != func.paramNamesAndTypes.size()) {
					foundButWrongArgCount = true;
					goto nextFunc;
				}
				map<string_view, ProofSyntax_MatchedValue> argsMap;
				for (unsigned int i=0; i<args.size(); i++) {
					auto& [paramName, paramType] = func.paramNamesAndTypes[i];
					if (!isOfType(args[i], paramType)) {
						foundButWrongArgTypes = true;
						goto nextFunc;
					}
					argsMap.insert({paramName, args[i]});
				}
				
				if (func.output.size() == 1) {
					return getOutputSegment(func.output[0], proofSyntax, argsMap, callDepth+1);
				} else {
					std::stringstream str;
					for (auto& outputSegment : func.output) {
						getOutputSegmentStr(outputSegment, proofSyntax, argsMap, str, callDepth+1);
					}
					return ProofSyntax_MatchedValue_AnyString(std::move(str).str());
				}
				
				/*
				if (std::holds_alternative<ProofSyntax_OutputSegment>(func.output)) {
					return getOutputSegment(std::get<ProofSyntax_OutputSegment>(func.output), proofSyntax, pr);
				} else if (std::holds_alternative<vector<ProofSyntax_OutputSegment>>(func.output)) {
					std::stringstream str;
					for (auto& outputSegment : std::get<vector<ProofSyntax_OutputSegment>>(func.output)) {
						getOutputSegmentStr(outputSegment, proofSyntax, pr, str);
					}
					return std::move(str).str();
				} else {
					throw 77791849814;
				}
				*/
			}
			nextFunc:;
		}
		std::println("\n\n{}\n", funcCall.funcName);
		if (foundButWrongArgCount) throw "No function overload with that argument count";
		else if (foundButWrongArgTypes) throw "No function overload with those arguments";
		else throw "Function does not exist";
	} else if (std::holds_alternative<ProofSyntax_OutputSegment_Variable>(outputSegment)) {
		string_view varName = std::get<ProofSyntax_OutputSegment_Variable>(outputSegment).varName;
		auto it = pr.find(varName);
		if (it == pr.end()) { std::println("\n\n{}\n", varName); throw "Var does not exist"; }
		return it->second;
	} else if (std::holds_alternative<ProofSyntax_OutputSegment_Sub>(outputSegment)) {
		auto& sub = std::get<ProofSyntax_OutputSegment_Sub>(outputSegment);
		auto lhs = getOutputSegment(*sub.lhs, proofSyntax, pr, callDepth+1);
		if (std::holds_alternative<ProofSyntax_MatchedValue_SubSyntax>(lhs)) {
			auto& pr = std::get<ProofSyntax_MatchedValue_SubSyntax>(lhs);
			auto it = pr.matches.find(sub.rhs);
			if (it == pr.matches.end()) {
				std::println("\n\nsubscript=.{}\nExisting subscripts:", sub.rhs);
				for (auto& [a, _b] : pr.matches) {
					std::println("{}", a);
				}
				throw "Subscript does not exist";
			}
			return it->second;
		} else {
			throw "Subscript on wrong kind of thing";
		}
	} else {
		throw 187512;
	}
}

void getOutputSegmentStr(
	const ProofSyntax_OutputSegment& outputSegment,
	const ProofSyntax& proofSyntax,
	const map<string_view, ProofSyntax_MatchedValue>& pr,
	std::stringstream& out
) {
	getOutputSegmentStr(outputSegment, proofSyntax, pr, out, 0);
}
static void getOutputSegmentStr(
	const ProofSyntax_OutputSegment& outputSegment,
	const ProofSyntax& proofSyntax,
	const map<string_view, ProofSyntax_MatchedValue>& pr,
	std::stringstream& out,
	unsigned int callDepth
) {
	if (std::holds_alternative<ProofSyntax_OutputSegment_Variable>(outputSegment)) {
		auto& var = std::get<ProofSyntax_OutputSegment_Variable>(outputSegment);
		auto it = pr.find(var.varName);
		//std::println("\n{}\n", var.varName);
		if (it == pr.end()) throw "Var does not exist?";
		if (std::holds_alternative<ProofSyntax_MatchedValue_AnyString>(it->second)) {
			out << std::get<ProofSyntax_MatchedValue_AnyString>(it->second).str;
		} else if (std::holds_alternative<ProofSyntax_MatchedValue_AnyStringView>(it->second)) {
			out << std::get<ProofSyntax_MatchedValue_AnyStringView>(it->second).str;
		} else if (std::holds_alternative<ProofSyntax_MatchedValue_Identifier>(it->second)) {
			out << std::get<ProofSyntax_MatchedValue_Identifier>(it->second).ident;
		} else if (std::holds_alternative<ProofSyntax_MatchedValue_SubSyntax>(it->second)) {
			out << std::get<ProofSyntax_MatchedValue_SubSyntax>(it->second).matchedString;
		} else {
			std::println("\n'{}'\n", var.varName);
			throw 8975372582538;
		}
	} else if (std::holds_alternative<ProofSyntax_OutputSegment_Sub>(outputSegment)) {
		auto& sub = std::get<ProofSyntax_OutputSegment_Sub>(outputSegment);
		auto lhs = getOutputSegment(*sub.lhs, proofSyntax, pr, callDepth+1);
		if (std::holds_alternative<ProofSyntax_MatchedValue_SubSyntax>(lhs)) {
			auto& pr = std::get<ProofSyntax_MatchedValue_SubSyntax>(lhs);
			auto it = pr.matches.find(sub.rhs);
			if (it == pr.matches.end()) {
				std::println("\n\nsubscript=.{}\nExisting subscripts:", sub.rhs);
				for (auto& [a, _b] : pr.matches) {
					std::println("{}", a);
				}
				throw "Subscript does not exist";
			}
			if (std::holds_alternative<ProofSyntax_MatchedValue_AnyString>(it->second)) {
				out << std::get<ProofSyntax_MatchedValue_AnyString>(it->second).str;
			} else if (std::holds_alternative<ProofSyntax_MatchedValue_AnyStringView>(it->second)) {
				out << std::get<ProofSyntax_MatchedValue_AnyStringView>(it->second).str;
			} else if (std::holds_alternative<ProofSyntax_MatchedValue_Identifier>(it->second)) {
				out << std::get<ProofSyntax_MatchedValue_Identifier>(it->second).ident;
			} else if (std::holds_alternative<ProofSyntax_MatchedValue_SubSyntax>(it->second)) {
				out << std::get<ProofSyntax_MatchedValue_SubSyntax>(it->second).matchedString;
			} else {
				throw "nope subscript nope";
			}
		} else {
			throw "Subscript on wrong kind of thing";
		}
	} else if (std::holds_alternative<ProofSyntax_OutputSegment_Literal>(outputSegment)) {
		out << std::get<ProofSyntax_OutputSegment_Literal>(outputSegment).strv();
	} else if (std::holds_alternative<ProofSyntax_OutputSegment_FunctionCall>(outputSegment)) {
		auto& funcCall = std::get<ProofSyntax_OutputSegment_FunctionCall>(outputSegment);
		//std::println("{}getOutputSegmentStr(): Function call on {} with {} args", std::string(callDepth*2, ' '), funcCall.funcName, funcCall.args.size());
		
		if (funcCall.funcName == "replace_ident"sv) {
			if (funcCall.args.size() != 3) throw SyntaxError("replace_ident must have 3 arguments"sv, FilePos{});
			std::stringstream from, to, subject;
			getOutputSegmentStr(*funcCall.args[0], proofSyntax, pr, from);
			getOutputSegmentStr(*funcCall.args[1], proofSyntax, pr, to);
			getOutputSegmentStr(*funcCall.args[2], proofSyntax, pr, subject);
			std::string from_    = std::move(from   ).str();
			std::string to_      = std::move(to     ).str();
			std::string subject_ = std::move(subject).str();
			Parser p(subject_);
			while (true) {
				if (p.areAtEnd()) break;
				auto posBeforeSkipWhitespace = p.currentFilePos();
				p.skipWhitespace();
				if (p.currentFilePos().index != posBeforeSkipWhitespace.index) {
					out << ' ';
					if (p.areAtEnd()) break;
				}
				if (auto ident = p.tryReadIdentifier()) {
					if (ident.value() == from_) out << to_;
					else out << ident.value();
				} else {
					out << p.readNonWhitespaceChar();
				}
			}
			return;
		}
		
		vector<ProofSyntax_MatchedValue> args;
		args.reserve(funcCall.args.size());
		for (auto& arg : funcCall.args) {
			args.push_back(getOutputSegment(*arg, proofSyntax, pr, callDepth+1));
		}
		
		if (funcCall.funcName == "error"sv) {
			if (funcCall.args.size() >= 1) {
				if (auto* lit=std::get_if<ProofSyntax_OutputSegment_Literal>(funcCall.args[0].get())) {
					std::visit([](auto&& str){throw SyntaxError(str, FilePos{});}, lit->strv_or_str);
				}
			}
			throw SyntaxError("error"sv, FilePos{});
		}
		
		if (funcCall.funcName == "concat"sv) {
			if (args.size() == 0) { }
			else if (args.size() == 1) {
				if (auto ident=std::get_if<ProofSyntax_MatchedValue_Identifier>(&args[0])) out << ident->ident;
				else if (auto s=std::get_if<ProofSyntax_MatchedValue_AnyString>(&args[0])) out << s->str;
				else if (auto sv=std::get_if<ProofSyntax_MatchedValue_AnyStringView>(&args[0])) out << sv->str;
				else if (auto sv=std::get_if<ProofSyntax_MatchedValue_SubSyntax>(&args[0])) out << sv->matchedString;
				else throw 1984983;
			} else {
				for (auto& arg : args) {
					if (auto ident=std::get_if<ProofSyntax_MatchedValue_Identifier>(&arg)) out << ident->ident;
					else if (auto s=std::get_if<ProofSyntax_MatchedValue_AnyString>(&arg)) out << s->str;
					else if (auto sv=std::get_if<ProofSyntax_MatchedValue_AnyStringView>(&arg)) out << sv->str;
					else if (auto sv=std::get_if<ProofSyntax_MatchedValue_SubSyntax>(&arg)) out << sv->matchedString;
					else throw 1984983;
				}
			}
			return;
		}
		
		bool foundButWrongArgCount = false;
		bool foundButWrongArgTypes = false;
		for (auto& func : proofSyntax.functions) {
			if (func.funcName == funcCall.funcName) {
				if (funcCall.args.size() != func.paramNamesAndTypes.size()) {
					foundButWrongArgCount = true;
					goto nextFunc;
				}
				map<string_view, ProofSyntax_MatchedValue> argsMap;
				for (unsigned int i=0; i<funcCall.args.size(); i++) {
					auto& [paramName, paramType] = func.paramNamesAndTypes[i];
					if (!isOfType(args[i], paramType)) {
						foundButWrongArgTypes = true;
						goto nextFunc;
					}
					argsMap.insert({paramName, args[i]});
				}
				for (auto& outputSegment : func.output) {
					getOutputSegmentStr(outputSegment, proofSyntax, argsMap, out, callDepth+1);
				}
				return;
			}
			nextFunc:;
		}
		std::println("\n\n{}\n", funcCall.funcName);
		if (foundButWrongArgCount) throw "No function overload with that argument count";
		else if (foundButWrongArgTypes) throw "No function overload with those arguments";
		else throw "Function does not exist";
	} else {
		throw 981298142;
	}
}

static shared_ptr<const Proof> parseProof(
	Namespace& ns,
	FilePos filePos,
	string_view str,
	unsigned int callDepth,
	bool syntaxAssumePermitted
) {
	std::println("parseProof() on: {}", str);
	
	if (str.size() == 0) throw SyntaxError("Failed to parse proof: empty string"sv, filePos);
	
	{
		Parser parser(str);
		parser.skipWhitespace();
		if (parser.tryReadKeyword("try"sv)) {
			parser.skipWhitespace();
			parser.readChar('(', "Expected ( after keyword 'try'"sv);
			vector<shared_ptr<const Proof>> tryList;
			while (true) {
				auto subProofStr = parser.readUntilCharOrEnd(',', true);
				Namespace ns2(&ns);
				tryList.push_back(parseProof(ns2, FilePos{}, subProofStr, callDepth+1, syntaxAssumePermitted));
				parser.skipWhitespace();
				if (parser.tryReadChar(',')) continue;
				else if (parser.tryReadChar(')')) break;
				else throw SyntaxError("Expected , or ) after proof after 'try ('"sv, FileRange::none());
			}
			parser.skipWhitespace();
			if (parser.areAtEnd()) {
				return std::make_shared<Proof_TryList>(FileRange::none(), std::move(tryList));
			}
		}
	}
	
	{
		Parser parser(str);
		if (parser.tryReadKeyword("substitute"sv)) {
			vector<pair<string_view, shared_ptr<const Expression>>> subsitutions;
			do {
				parser.skipWhitespace();
				auto varName = parser.tryReadIdentifier();
				if (!varName.has_value()) {
					throw SyntaxError("Expected identifier (variable to be substituted) or keyword 'in' in substitution list", parser.currentFilePos());
				}
				parser.skipWhitespace();
				if (!parser.tryReadChar('=')) {
					throw SyntaxError("Expected = after variable name in substitution list", parser.currentFilePos());
				}
				parser.skipWhitespace();
				auto expr = parser.readExpression(ns, ',');
				parser.skipWhitespace();
				subsitutions.emplace_back(varName.value(), std::move(expr));
			} while (parser.tryReadChar(','));
			if (!parser.tryReadKeyword("in"sv)) {
				throw SyntaxError("Expected keyword 'in' after substitution list", parser.currentFilePos());
			}
			parser.skipWhitespace();
			auto subProof = parseProof(ns, FilePos{}, parser.str, callDepth+1, syntaxAssumePermitted);
			return std::make_shared<Proof_Substitute>(FileRange::none(), std::move(subsitutions), std::move(subProof));
		}
	}

	{
		Parser parser(str);
		parser.skipWhitespace();
		auto ident = parser.tryReadIdentifier();
		if (ident.has_value()) {
			parser.skipWhitespace();
			if (parser.areAtEnd()) {
				auto maybeId = ns.find(ident.value());
				if (maybeId.has_value()) {
					return std::make_shared<Proof_Id>(FileRange::none(), maybeId.value());
				} else {
					std::println("\nidentifier='{}'\n", ident.value());
					throw SyntaxError("Unknown identifier"sv, FileRange::none());
				}
			}
		}
	}
	
	{
		vector<string_view> subProofs;
		Parser parser(str);
		while (true) {
			auto lhs = parser.readUntilCharOrEnd('>', true);
			if (lhs.size() == 0) goto nope;
			if (parser.areAtEnd()) {
				if (subProofs.size() == 0) goto nope;
				else { subProofs.push_back(lhs); break; }
			} else {
				if (isOperatorChar(lhs.back())) goto nope;
				parser.readChar('>', "wtf"sv);
				if (parser.areAtEnd()) goto nope;
				if (isOperatorChar(parser.str[0])) goto nope;
				subProofs.push_back(lhs);
			}
		}
		
		{
			auto ret = parseProof(ns, FilePos(0), subProofs.back(), callDepth+1, syntaxAssumePermitted);
			for (int i=subProofs.size()-2; i>=0; i--) {
				ret = std::make_shared<Proof_Shove>(FileRange::firstLast(filePos, filePos), parseProof(ns, FilePos(0), subProofs[i], callDepth+1, syntaxAssumePermitted), std::move(ret));
			}
			return ret;
		}
		nope:;
	}
	
	
	{
		vector<string_view> subProofs;
		Parser parser(str);
		while (true) {
			auto lhs = parser.readUntilCharOrEnd('<', true);
			if (lhs.size() == 0) goto nope2;
			if (parser.areAtEnd()) {
				if (subProofs.size() == 0) goto nope2;
				else { subProofs.push_back(lhs); break; }
			} else {
				if (isOperatorChar(lhs.back())) goto nope2;
				parser.readChar('<', "wtf"sv);
				if (parser.areAtEnd()) goto nope2;
				if (isOperatorChar(parser.str[0])) goto nope2;
				subProofs.push_back(lhs);
			}
		}
		
		{
			auto ret = parseProof(ns, FilePos(0), subProofs[0], callDepth+1, syntaxAssumePermitted);
			for (int i=1; i<subProofs.size(); i++) {
				ret = std::make_shared<Proof_Shove>(FileRange::firstLast(filePos, filePos), parseProof(ns, FilePos(0), subProofs[i], callDepth+1, syntaxAssumePermitted), std::move(ret));
			}
			return ret;
		}
		nope2:;
	}
	
	
	{
		Parser parser(str);
		parser.skipWhitespace();
		if (parser.tryReadChar('(')) {
			//auto contentFilePos = parser.currentFilePos();
			auto content = parser.readUntilCharOrEnd(')', false);
			parser.readChar(')', "No ) to match ("sv);
			auto contentWithClosingParen = string_view(&content[0], content.size()+1);
			parser.skipWhitespace();
			if (parser.areAtEnd()) {
				Parser subParser(contentWithClosingParen);
				std::println("content='{}'", content);
				Namespace ns2(&ns);
				vector<shared_ptr<const Statement>> statements;
				auto proof = subParser.readStatementsAndMaybeOneProof(ns2, statements, syntaxAssumePermitted);
				subParser.skipWhitespace();
				subParser.readChar(')', "Wtf"sv);
				if (!subParser.areAtEnd()) {
					std::println("\n{}\n", content);
					throw SyntaxError("Unexpected something before )"sv, FileRange::none());
				}
				if (proof == nullptr) throw SyntaxError("Expected proof before )"sv, FileRange::none());
				return std::make_shared<Proof_Block>(FileRange::none(), std::move(statements), std::move(proof));
			}
		}
	}
	
	{
		Parser parser(str);
		//parser.line = filePos.line;
		//parser.col = filePos.col;
		//parser.index = TODO
		parser.skipWhitespace();
		auto startPos = parser.currentFilePos();
		for (auto& proofSyntax : ns.getProofSyntaxes()) {
			map<string_view, ProofSyntax_MatchedValue> parseResult;
			if (tryReadProofByCustomSyntax(parser, ns, proofSyntax.mainParseDefinition, proofSyntax, parseResult, {NothingHere{}}, callDepth+1)) {
				parser.skipWhitespace();
				if (parser.areAtEnd()) {
					std::stringstream newString;
					for (auto& outputSegment : proofSyntax.mainOutput) {
						getOutputSegmentStr(outputSegment, proofSyntax, parseResult, newString);
					}
					std::string* newString_ = new std::string(std::move(newString).str()); // TODO: leak
					std::print("\nCustom proofsyntax gave:\n{}\n", *newString_);
					return parseProof(ns, filePos, *newString_, callDepth+1, proofSyntax.isAssumed);
				}
			}
			parser.rewindTo(startPos);
		}
	}
	
	throw SyntaxError("Failed to parse proof"sv, filePos);
}


[[nodiscard]] shared_ptr<const Proof> Parser::readProof(
	Namespace& ns,
	char endChar,
	bool syntaxAssumePermitted
) {
	skipWhitespace();
	
	auto pos = currentFilePos();
	auto proofStr = readUntilCharOrEnd(endChar, true);
	return parseProof(ns, pos, proofStr, 0, syntaxAssumePermitted);
}
