module;

#include <strings.h>

export module BusLang:Namespace;

import std;
import :Id;
import :FileRange;
import :SyntaxError;
import :Syntax;
import :ProofSyntax;

export struct Namespace {
private:
	std::map<std::string_view, std::pair<Id, FileRange>> name_to_id_and_fileRange;
	std::vector<size_t> insertedSyntaxIndices;
	std::vector<size_t> insertedProofSyntaxIndices;
	Namespace* maybeParent;
	std::vector<Syntax>& syntaxes;
	std::vector<ProofSyntax>& proofSyntaxes;
public:
	constexpr Namespace(std::vector<Syntax>& _syntaxes, std::vector<ProofSyntax>& _proofSyntaxes):
		maybeParent(nullptr), syntaxes(_syntaxes), proofSyntaxes(_proofSyntaxes) { }
	constexpr Namespace(Namespace* _parent): maybeParent(_parent), syntaxes(_parent->syntaxes), proofSyntaxes(_parent->proofSyntaxes) { }
	Namespace(Namespace&&) = delete;
	Namespace(const Namespace&) = delete;
	Namespace& operator = (Namespace&&) = delete;
	Namespace& operator = (const Namespace&) = delete;
	~Namespace() {
		// TODO: This is inefficient
		for (auto it=insertedSyntaxIndices.rbegin(); it != insertedSyntaxIndices.rend(); it++) {
			syntaxes.erase(syntaxes.begin() + *it);
		}
		for (auto it=insertedProofSyntaxIndices.rbegin(); it != insertedProofSyntaxIndices.rend(); it++) {
			proofSyntaxes.erase(proofSyntaxes.begin() + *it);
		}
	}
	
	void addSyntax(Syntax&& syntax) {
		unsigned int i = 0;
		while (i < syntaxes.size() && syntaxes[i].precedence < syntax.precedence) i++;
		syntaxes.insert(syntaxes.begin() + i, std::move(syntax));
		insertedSyntaxIndices.push_back(i);
	}
	void addProofSyntax(ProofSyntax&& proofSyntax) {
		// TODO: maybe add precedence sorting for proofsyntaxes?
		insertedProofSyntaxIndices.push_back(proofSyntaxes.size());
		proofSyntaxes.push_back(std::move(proofSyntax));
	}
	
	[[nodiscard]] constexpr const std::vector<Syntax>& getSyntaxes() const noexcept { return syntaxes; }
	[[nodiscard]] constexpr const std::vector<ProofSyntax>& getProofSyntaxes() const noexcept { return proofSyntaxes; }
	
	[[nodiscard]] std::optional<Id> find(std::string_view name) const {
		if (auto it = name_to_id_and_fileRange.find(name); it != name_to_id_and_fileRange.end()) {
			return it->second.first;
		} else if (maybeParent != nullptr) {
			return maybeParent->find(name);
		} else {
			return std::nullopt;
		}
	}
	
	[[nodiscard]] Id make(std::string_view name, FileRange fileRange) {
		auto it = name_to_id_and_fileRange.find(name);
		if (it != name_to_id_and_fileRange.end()) {
			throw SyntaxError("Name already exists in current namespace!", fileRange, it->second.second);
		} else {
			Id newId = Id::make(name);
			name_to_id_and_fileRange.insert({name, {newId, fileRange}});
			return newId;
		}
	}
	
	void add(Id id) {
		if (auto it = name_to_id_and_fileRange.find(id.name); it != name_to_id_and_fileRange.end()) {
			throw SyntaxError("Name already exists in current namespace!", FileRange::none(), it->second.second);
		}
		name_to_id_and_fileRange.emplace(id.name, std::make_pair(id, FileRange::none()));
	}
};
