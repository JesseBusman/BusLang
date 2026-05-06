#pragma once

#include <string_view>
#include <map>
#include <vector>
#include <optional>

#include "file_range.h"
#include "id.h"
#include "proofsyntax.h"
#include "syntax.h"


struct Namespace {
private:
	friend int main(int nargs, char** args);
	std::map<std::string_view, std::pair<Id, FileRange>> name_to_id_and_fileRange;
	constexpr Namespace(std::vector<Syntax>& _syntaxes, std::vector<ProofSyntax>& _proofSyntaxes):
		maybeParent(nullptr), syntaxes(_syntaxes), proofSyntaxes(_proofSyntaxes) { }
	std::vector<size_t> insertedSyntaxIndices;
	std::vector<size_t> insertedProofSyntaxIndices;
	Namespace* maybeParent;
	std::vector<Syntax>& syntaxes;
	std::vector<ProofSyntax>& proofSyntaxes;
public:
	constexpr Namespace(Namespace* _parent): maybeParent(_parent), syntaxes(_parent->syntaxes), proofSyntaxes(_parent->proofSyntaxes) { }
	Namespace(Namespace&&) = delete;
	Namespace(const Namespace&) = delete;
	Namespace& operator = (Namespace&&) = delete;
	Namespace& operator = (const Namespace&) = delete;
	~Namespace();
	
	void addSyntax(Syntax&& syntax);
	void addProofSyntax(ProofSyntax&& proofSyntax);
	
	[[nodiscard]] constexpr const std::vector<Syntax>& getSyntaxes() const noexcept { return syntaxes; }
	[[nodiscard]] constexpr const std::vector<ProofSyntax>& getProofSyntaxes() const noexcept { return proofSyntaxes; }
	
	[[nodiscard]] std::optional<Id> find(std::string_view name) const;
	
	[[nodiscard]] Id make(std::string_view name, FileRange fileRange);
	
	void add(Id id);
};
