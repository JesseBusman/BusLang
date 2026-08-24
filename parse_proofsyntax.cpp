module BusLang;

import std;
import :Parser;

ProofSyntax_Type Parser::readParseType() {
	skipWhitespace();
	auto ident = readIdentifier("Expected parse type"sv);
	if (ident == "IDENT"sv) return ProofSyntax_Type_Identifier();
	if (ident == "PAREN"sv) return ProofSyntax_Type_Parentheses();
	//if (ident == "ANY"sv) return ProofSyntax_Type_Any();
	skipWhitespace();
	if (tryReadChar('.')) {
		skipWhitespace();
		auto alternativeIndex = readInteger("Expected integer"sv);
		return ProofSyntax_Type_SubParseSpecificAlternative(ident, alternativeIndex);
	} else {
		return ProofSyntax_Type_SubParse(ident);
	}
}

ProofSyntax_ParsePiece Parser::readParsePiece() {
	skipWhitespace();
	if (tryPeekChar(')')) { std::println("\n'{}'\n", this->str.substr(0, 100)); throw "wtf 983589235"; }
	if (tryReadChar(';')) {
		return ProofSyntax_ParsePiece_Semicolon();
	} else if (tryReadChar('(')) {
		skipWhitespace();
		std::string_view name;
		if (auto maybeName=tryReadIdentifier(); maybeName.has_value()) {
			name = maybeName.value();
			skipWhitespace();
			if (tryReadChar(')')) {
				return ProofSyntax_ParsePiece_Typed(name, ProofSyntax_Type_Any());
			}
		}
		readChar(':', "Expected :"sv);
		skipWhitespace();
		auto type = readParseType();
		skipWhitespace();
		readChar(')', "Expected )"sv);
		return ProofSyntax_ParsePiece_Typed(name, std::move(type));
	} else if (auto keyword=tryReadIdentifier()) {
		return ProofSyntax_ParsePiece_Keyword(keyword.value());
	} else {
		auto startPos = currentFilePos();
		while (!areAtEnd()) {
			if (!isOperatorChar(str[0])) break;
			auto _ = readNonWhitespaceChar();
		}
		if (startPos.index == currentFilePos().index) {
			std::println("\n'{}'\n", str.substr(0, 100));
			throw SyntaxError("Expected ( or ) or keyword or operator"sv, currentFilePos());
		} else {
			return ProofSyntax_ParsePiece_OperatorChars(getStringViewFromTo(startPos, currentFilePos()));
		}
	}
}

ProofSyntax_OutputSegment Parser::readStuffInDoubleParens_sub() {
	skipWhitespace();
	auto startPos = currentFilePos();
	if (tryReadChar('"')) {
		std::stringstream ret;
		bool backslashing = false;
		while (true) {
			if (areAtEnd()) throw SyntaxError("Expected \" to match \""sv, startPos, currentFilePos());
			char c = str[0];
			if (c == '\n' || c == '\r') throw SyntaxError("Encountered newline inside string literal, please use \\n and/or \\r", currentFilePos());
			str = str.substr(1);
			col++;
			if (backslashing) {
				if (c == 'r') ret << '\r';
				else if (c == 'n') ret << '\n';
				else if (c == 't') ret << '\t';
				else ret << c;
				backslashing = false;
			} else {
				if (c == '\\') backslashing = true;
				else if (c == '"') break;
				else ret << c;
			}
		}
		return ProofSyntax_OutputSegment_Literal(std::move(ret).str());
	}
	ProofSyntax_OutputSegment ret = [this]()->ProofSyntax_OutputSegment{
		if (tryReadChar('(')) {
			auto ret = readStuffInDoubleParens();
			skipWhitespace();
			readChar(')', "Expected )"sv);
			return ret;
		} else {
			return ProofSyntax_OutputSegment_Variable{.varName = readIdentifier("Expected identifier"sv)};
		}
	}();
	skipWhitespace();
	while (true) {
		if (tryReadChar('.')) {
			skipWhitespace();
			auto ident = readIdentifier("Expected identifier"sv);
			ret = ProofSyntax_OutputSegment_Sub{
				.lhs = std::make_shared<ProofSyntax_OutputSegment>(std::move(ret)),
				.rhs = ident
			};
		} else {
			return ret;
		}
		skipWhitespace();
	}
}

ProofSyntax_OutputSegment Parser::readStuffInDoubleParens() {
	skipWhitespace();
	auto firstStuff = readStuffInDoubleParens_sub();
	skipWhitespace();
	if (tryPeekChar(',') || tryPeekChar(')')) return firstStuff;
	if (!std::holds_alternative<ProofSyntax_OutputSegment_Variable>(firstStuff)) throw SyntaxError("Expected , or )"sv, currentFilePos());
	
	vector<shared_ptr<ProofSyntax_OutputSegment>> funcArgs;
	while (true) {
		funcArgs.push_back(std::make_shared<ProofSyntax_OutputSegment>(readStuffInDoubleParens_sub()));
		skipWhitespace();
		if (tryPeekChar(',') || tryPeekChar(')')) break;
	}
	
	return ProofSyntax_OutputSegment_FunctionCall{
		.funcName = std::get<ProofSyntax_OutputSegment_Variable>(firstStuff).varName, .args = std::move(funcArgs)
	};
}


ProofSyntax_OutputSegment Parser::readSyntaxOutputSegment(unsigned int& depth) {
	if (str.size() == 0) throw SyntaxError("File ended in the middle of syntax output spec"sv, currentFilePos());
	//if (tryPeekChar(')')) throw SyntaxError("Unexpected )"sv, currentFilePos());
	
	auto tryReadSyntaxOutputSegment_doubleParen = [this]() -> optional<ProofSyntax_OutputSegment> {
		auto startPos = currentFilePos();
		
		if (tryReadChar('(')) {
			if (tryReadChar('(')) {
				auto startPos2 = currentFilePos();
				while (true) {
					skipWhitespace();
					if (tryPeekChar('(')) {
						skipParenEnclosedStuff(false);
					} else if (str.size() >= 2 && str[0] == ')' && str[1] == ')') {
						break;
					} else if (tryPeekChar(')')) {
						rewindTo(startPos);
						return std::nullopt;
					} else {
						auto _ = readNonWhitespaceChar();
					}
				}
				
				rewindTo(startPos2);
				
				auto stuff = readStuffInDoubleParens();
				
				readChar(')', "Expected ))"sv);
				readChar(')', "Expected )"sv);
				
				return stuff;
			}
		}
		
		rewindTo(startPos);
		
		return std::nullopt;
	};
	
	if (auto doubleParenContent=tryReadSyntaxOutputSegment_doubleParen(); doubleParenContent.has_value()) {
		return std::move(doubleParenContent.value());
	}
	
	auto startPos = currentFilePos();
	while (true) {
		skipWhitespace();
		if (areAtEnd()) throw SyntaxError("Unexpected end of file"sv, currentFilePos());
		auto beforePos = currentFilePos();
		if (tryReadSyntaxOutputSegment_doubleParen().has_value()) {
			rewindTo(beforePos);
			break;
		}
		if (tryReadChar('(')) depth++;
		else if (depth == 0 && tryPeekChar(')')) break;
		else if (tryReadChar(')')) depth--;
		else { auto _ = readNonWhitespaceChar(); }
	}
	
	auto literal = getStringViewFromTo(startPos, currentFilePos());
	//std::println("literal='{}' depth={}", literal, depth);
	return ProofSyntax_OutputSegment_Literal(literal);
}


shared_ptr<ProofSyntax> Parser::readProofSyntax(bool assumed) {
	map<std::string_view, ProofSyntax_SubParseDefinition> subParseDefinitions;
	vector<ProofSyntax_Function> functions;
	optional<vector<ProofSyntax_ParsePiece>> mainParseDefinition;
	optional<vector<ProofSyntax_OutputSegment>> mainOutput;
	
	while (true) {
		if (tryReadChar(';')) break;
		skipWhitespace();
		if (tryReadChar(',')) { 
			skipWhitespace();
			if (tryReadChar(';')) break;
		}
		if (auto maybeIdent=tryReadIdentifier(); maybeIdent.has_value()) {
			skipWhitespace();
			if (tryReadChar(':')) {
				// It's a type
				skipWhitespace();
				readChar('(', "Expected ( after :"sv);
				
				vector<vector<ProofSyntax_ParsePiece>> alternatives;
				while (true) {
					skipWhitespace();
					readChar('(', "Expected ("sv);
					
					vector<ProofSyntax_ParsePiece> parsePieces;
					std::println("Reading sub syntax");
					while (true) {
						skipWhitespace();
						if (tryReadChar(')')) break;
						parsePieces.push_back(readParsePiece());
					}
					alternatives.push_back(std::move(parsePieces));
					std::println("Read sub syntax: {} pieces", parsePieces.size());
					
					skipWhitespace();
					if (!tryReadChar(',')) {
						readChar(')', "Expected , or )"sv);
						break;
					}
					skipWhitespace();
					if (tryReadChar(')')) break;
				}
				
				subParseDefinitions[maybeIdent.value()] = ProofSyntax_SubParseDefinition(std::move(alternatives));
				
			} else {
				// It's a function
				
				vector<pair<string_view, ProofSyntax_Type>> paramNamesAndTypes;
				
				while (true) {
					skipWhitespace();
					if (areAtEnd()) throw SyntaxError("Unexpected end of file"sv, currentFilePos());
					if (tryReadChar('=')) break;
					
					if (auto ident = tryReadIdentifier(); ident.has_value()) {
						paramNamesAndTypes.emplace_back(ident.value(), ProofSyntax_Type_Any());
					} else {
						readChar('(', "Expected = or identifier or ("sv);
						skipWhitespace();
						auto paramName = readIdentifier("Expected identifier (parameter name)"sv);
						skipWhitespace();
						readChar(':', "Expected : followed by parameter type"sv);
						skipWhitespace();
						auto paramType = readParseType();
						skipWhitespace();
						readChar(')', "Expected ) after parameter type"sv);
						paramNamesAndTypes.emplace_back(paramName, std::move(paramType));
					}
				}
				
				skipWhitespace();
				if (tryReadChar('(')) {
					
					unsigned int depth = 0;
					vector<ProofSyntax_OutputSegment> outputSegments;
					
					while (true) {
						if (areAtEnd()) throw SyntaxError("Unexpected end of file"sv, currentFilePos());
						if (depth == 0 && tryReadChar(')')) break;
						outputSegments.push_back(readSyntaxOutputSegment(depth));
					}
					
					functions.emplace_back(
						maybeIdent.value(),
						std::move(paramNamesAndTypes),
						std::move(outputSegments)
					);
				} else {
					throw SyntaxError("Expected ( after ="sv, currentFilePos());
					/*unsigned int depth = 0;
					auto output = readSyntaxOutputSegment(depth);
					
					functions.emplace_back(
						maybeIdent.value(),
						std::move(paramNamesAndTypes),
						std::move(output)
					);*/
				}
			}
		} else if (tryReadChar('(')) {
			if (mainParseDefinition.has_value()) throw SyntaxError("Only one main parse definition is allowed"sv, currentFilePos());
			
			// It's the main syntax
			
			std::println("Reading main syntax...");
			
			skipWhitespace();
			
			vector<ProofSyntax_ParsePiece> parsePieces;
			while (true) {
				parsePieces.push_back(readParsePiece());
				skipWhitespace();
				if (tryReadChar(')')) break;
			}
			std::println("Read main syntax: {} pieces", parsePieces.size());
			skipWhitespace();
			readChar('=', "Expected ="sv);
			skipWhitespace();
			readChar('(', "Expected ("sv);
			
			mainParseDefinition = std::move(parsePieces);
			
			unsigned int depth = 0;
			vector<ProofSyntax_OutputSegment> outputSegments;
			while (true) {
				//std::println("Reading output segment from '{}'", this->str.substr(0, 100));
				if (areAtEnd()) throw SyntaxError("Unexpected end of file"sv, currentFilePos());
				if (depth == 0 && tryReadChar(')')) break;
				outputSegments.push_back(readSyntaxOutputSegment(depth));
			}
			
			mainOutput = std::move(outputSegments);
			
			//std::println("Read main output: {} segments", outputSegments.size());
		} else {
			throw SyntaxError("Expected identifier or ( inside proof syntax"sv, currentFilePos());
		}
	}
	
	if (!mainParseDefinition.has_value()) throw SyntaxError("No main parse definition in proofsyntax"sv, currentFilePos());
	
	return std::make_shared<ProofSyntax>(ProofSyntax{
		.subParseDefinitions = std::move(subParseDefinitions),
		.functions = std::move(functions),
		.mainParseDefinition = std::move(mainParseDefinition.value()),
		.mainOutput = std::move(mainOutput.value()),
		.isAssumed = assumed,
	});
}
