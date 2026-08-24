#include <stdlib.h>

import std;
import BusLang;

using std::string_view_literals::operator""sv;
using std::vector;
using std::map;
using std::pair;
using std::shared_ptr;
using std::string_view;


[[nodiscard]] std::string readFile(const std::string& path)
{
	std::ifstream f(path, std::ios::binary | std::ios::ate);
	if (!f) throw "Cannot open file " + path;
	
	auto size = f.tellg();
	if (size <= 0) return {};
	
	std::string s(size, '\0');
	f.seekg(0);
	f.read(s.data(), size);
	
	return s;
}


int main(int nargs, char** args) {
	if (nargs < 2) {
		std::println("Specify filename");
		exit(1);
		return 1;
	}
	
	try {
		std::vector<Syntax> syntaxes;
		{
			auto leftId = Id::make("a"sv);
			auto rightId = Id::make("b"sv);
			syntaxes.emplace_back(Syntax{
				.name_to_syntaxAndExpr = {},
				.syntax = {leftId, rightId},
				.expr = std::make_shared<Expression_Apply>(
					FileRange::none(),
					std::make_shared<Expression_Id>(FileRange::none(), leftId),
					std::make_shared<Expression_Id>(FileRange::none(), rightId)
				),
				.precedence = 10000,
				.associativity = LEFT,
			});
		}
		
		std::vector<ProofSyntax> proofSyntaxes;
		
		Namespace rootNamespace(syntaxes, proofSyntaxes);
		ATOM_IMPLIES = std::make_shared<Expression_Id>(FileRange::none(), rootNamespace.make("IMPLIES"sv, FileRange::none()));
		
		std::string filename = args[1];
		std::string code = readFile(filename);
		
		/*unsigned int line = 1;
		unsigned int col = 1;
		{
			bool prevWasCR = false;
			for (char c : code) {
				if (c == '\r') {
					line++;
					col = 1;
					prevWasCR = true;
				} else {
					if (c == '\n') {
						if (!prevWasCR) {
							line++;
							col = 1;
						}
					} else {
						col++;
					}
					prevWasCR = false;
				}
			}
		}
		
		FilePos codeStart = FilePos{.index = 0, .line = 1, .col = 1};
		FilePos codeEnd = FilePos{.index = (unsigned int)code.size(), .line = line, .col = col};*/
		
		Parser parser(code/*, {TextSource{.range = FileRange::startEnd(codeStart, codeEnd), .source = FileRange::startEnd(codeStart, codeEnd)}}*/);
		
		vector<shared_ptr<const Statement>> statements;
		shared_ptr<const Proof> finalProofExpr;
		
		try {
			std::println("Parsing...");
			finalProofExpr = parser.readStatementsAndMaybeOneProof(rootNamespace, statements, false);
			parser.skipWhitespace();
			std::println("Parsed successfully, {} chars left unparsed!", parser.str.size());
		} catch (const SyntaxError& se) {
			if (!se.fileRange2.has_value()) {
				std::print("\n\n\nSyntax error at {}:{}:{}:\n   {}\n\n\n", filename, se.fileRange1.start.line, se.fileRange1.start.col, se.message);
			} else {
				std::print("\n\n\nSyntax error at {}:{}:{} and {}:{}:{}:\n   {}\n\n\n", filename, se.fileRange1.start.line, se.fileRange1.start.col, filename, se.fileRange2->start.line, se.fileRange2->start.col, se.message);
			}
			exit(1);
			return 1;
		}
		map<Id, shared_ptr<const Expression>> proofId_to_provenProp;
		
		try {
			vector<Id> proofIdsAdded;
			vector<Id> forAnyVarsIntroduced;
			vector<shared_ptr<const Expression>> assumptionsIntroduced;
			std::println("Evaluating...");
			runStatements(statements, proofId_to_provenProp, proofIdsAdded, forAnyVarsIntroduced, assumptionsIntroduced, true);
		} catch (const ProofError& pe) {
			if (pe.fileRange.length != 0 && pe.fileRange2.length != 0 && pe.fileRange3.length != 0) {
				std::print("\n\n\nProof error at {}:{}:{},  {}:{}:{},  {}:{}:{}:\n   {}\n\n\n",
					filename, pe.fileRange.start.line, pe.fileRange.start.col, filename, pe.fileRange2.start.line, pe.fileRange2.start.col, filename, pe.fileRange3.start.line, pe.fileRange3.start.col, pe.message);
			} else if (pe.fileRange.length != 0 && pe.fileRange2.length != 0) {
				std::print("\n\n\nProof error at {}:{}:{},  {}:{}:{}:\n   {}\n\n\n", filename, pe.fileRange.start.line, pe.fileRange.start.col, filename, pe.fileRange2.start.line, pe.fileRange2.start.col, pe.message);
			} else if (pe.fileRange.length != 0) {
				std::print("\n\n\nProof error at {}:{}:{}:\n   {}\n\n\n", filename, pe.fileRange.start.line, pe.fileRange.start.col, pe.message);
			} else {
				std::print("\n\n\nProof error in {}:\n   {}\n\n\n", filename, pe.message);
			}
		}
	} catch (int i) {
		std::print("\n\nERROR: {}\n\n", i);
		exit(1);
		return 1;
	} catch (long i) {
		std::print("\n\nERROR: {}\n\n", i);
		exit(1);
		return 1;
	} catch (const char* msg) {
		std::print("\n\nERROR: {}\n\n", msg);
		exit(1);
		return 1;
	} catch (const std::string& msg) {
		std::print("\n\nERROR: {}\n\n", msg);
		exit(1);
		return 1;
	} catch (std::string_view msg) {
		std::print("\n\nERROR: {}\n\n", msg);
		exit(1);
		return 1;
	}
}
