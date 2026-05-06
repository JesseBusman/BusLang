#include <string_view>
#include <optional>

#include "syntax_error.h"
#include "id.h"
#include "namespace.h"
#include "file_range.h"

[[nodiscard]] std::optional<Id> Namespace::find(std::string_view name) const {
	if (auto it = name_to_id_and_fileRange.find(name); it != name_to_id_and_fileRange.end()) {
		return it->second.first;
	} else if (maybeParent != nullptr) {
		return maybeParent->find(name);
	} else {
		return std::nullopt;
	}
}

[[nodiscard]] Id Namespace::make(std::string_view name, FileRange fileRange) {
	auto it = name_to_id_and_fileRange.find(name);
	if (it != name_to_id_and_fileRange.end()) {
		throw SyntaxError("Name already exists in current namespace!", fileRange, it->second.second);
	} else {
		Id newId = Id::make(name);
		name_to_id_and_fileRange.insert({name, {newId, fileRange}});
		return newId;
	}
}

void Namespace::addSyntax(Syntax&& syntax) {
	unsigned int i = 0;
	while (i < syntaxes.size() && syntaxes[i].precedence < syntax.precedence) i++;
	syntaxes.insert(syntaxes.begin() + i, std::move(syntax));
	insertedSyntaxIndices.push_back(i);
}

void Namespace::addProofSyntax(ProofSyntax&& proofSyntax) {
	// TODO: maybe add precedence sorting for proofsyntaxes?
	insertedProofSyntaxIndices.push_back(proofSyntaxes.size());
	proofSyntaxes.push_back(std::move(proofSyntax));
}

void Namespace::add(Id id) {
	if (auto it = name_to_id_and_fileRange.find(id.name); it != name_to_id_and_fileRange.end()) {
		throw SyntaxError("Name already exists in current namespace!", FileRange::none(), it->second.second);
	}
	name_to_id_and_fileRange.emplace(id.name, std::make_pair(id, FileRange::none()));
}

Namespace::~Namespace() {
	// TODO: This is inefficient
	for (auto it=insertedSyntaxIndices.rbegin(); it != insertedSyntaxIndices.rend(); it++) {
		syntaxes.erase(syntaxes.begin() + *it);
	}
	for (auto it=insertedProofSyntaxIndices.rbegin(); it != insertedProofSyntaxIndices.rend(); it++) {
		proofSyntaxes.erase(proofSyntaxes.begin() + *it);
	}
}
