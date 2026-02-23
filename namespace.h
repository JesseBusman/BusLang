#pragma once

#include <string_view>
#include <map>
#include <vector>
#include <optional>

#include "file_range.h"
#include "id.h"

#include "syntax.h"


struct Namespace {
private:
	friend int main(int nargs, char** args);
	std::map<std::string_view, std::pair<Id, FileRange>> name_to_id_and_fileRange;
	constexpr Namespace(std::vector<Syntax>& _syntaxes): maybeParent(nullptr), syntaxes(_syntaxes) { }
	std::vector<size_t> insertedSyntaxIndices;
	Namespace* maybeParent;
	std::vector<Syntax>& syntaxes;
public:
	constexpr Namespace(Namespace* _parent): maybeParent(_parent), syntaxes(_parent->syntaxes) { }
	Namespace(Namespace&&) = delete;
	Namespace(const Namespace&) = delete;
	Namespace& operator = (Namespace&&) = delete;
	Namespace& operator = (const Namespace&) = delete;
	~Namespace();
	
	void addSyntax(Syntax&& syntax);
	[[nodiscard]] constexpr const std::vector<Syntax>& getSyntaxes() const noexcept { return syntaxes; }
	
	[[nodiscard]] std::optional<Id> find(std::string_view name) const;
	
	[[nodiscard]] Id make(std::string_view name, FileRange fileRange);
};
