#pragma once

#include <memory>
#include <vector>
#include <print>
#include <string_view>

#include "file_range.h"
#include "id.h"

using std::vector;
using std::string_view;
using std::shared_ptr;
using std::pair;

struct Statement;
struct Expression;


struct Proof : std::enable_shared_from_this<Proof> {
	FileRange fileRange;
	constexpr Proof(FileRange _fileRange): fileRange(_fileRange) { }
	virtual void print() const = 0;
	virtual ~Proof() = default;
};


struct Proof_Id final : Proof {
	Id id;
	constexpr Proof_Id(FileRange _fileRange, Id _id): Proof(_fileRange), id(_id) { }
	void print() const override;
	inline ~Proof_Id() override = default;
};


struct Proof_Block final : Proof {
	vector<shared_ptr<const Statement>> statements;
	shared_ptr<const Proof> finalProof;
	constexpr Proof_Block(FileRange _fileRange, vector<shared_ptr<const Statement>>&& _statements, shared_ptr<const Proof>&& _finalProof):
		Proof(_fileRange), statements(std::move(_statements)), finalProof(std::move(_finalProof)) { }
	void print() const override;
	inline ~Proof_Block() override = default;
};


struct Proof_Unwrap final : Proof {
	Id defId;
	std::optional<unsigned int> patternIndex;
	shared_ptr<const Proof> subProof;
	constexpr Proof_Unwrap(FileRange _fileRange, Id _defId, std::optional<unsigned int> _patternIndex, shared_ptr<const Proof>&& _subProof):
		Proof(_fileRange), defId(_defId), patternIndex(_patternIndex), subProof(std::move(_subProof))
	{
	}
	void print() const override;
	inline ~Proof_Unwrap() override = default;
};

struct Proof_RawUnwrap final : Proof {
	Id defId;
	shared_ptr<const Proof> subProof;
	constexpr Proof_RawUnwrap(FileRange _fileRange, Id _defId, shared_ptr<const Proof>&& _subProof):
		Proof(_fileRange), defId(_defId), subProof(std::move(_subProof))
	{
	}
	void print() const override;
	inline ~Proof_RawUnwrap() override = default;
};

struct Proof_Wrap final : Proof {
	Id defId;
	std::optional<unsigned int> patternIndex;
	shared_ptr<const Proof> subProof;
	constexpr Proof_Wrap(FileRange _fileRange, Id _defId, std::optional<unsigned int> _patternIndex, shared_ptr<const Proof>&& _subProof):
		Proof(_fileRange), defId(_defId), patternIndex(_patternIndex), subProof(std::move(_subProof))
	{
	}
	void print() const override;
	inline ~Proof_Wrap() override = default;
};

struct Proof_RawWrap final : Proof {
	Id defId;
	shared_ptr<const Proof> subProof;
	constexpr Proof_RawWrap(FileRange _fileRange, Id _defId, shared_ptr<const Proof>&& _subProof):
		Proof(_fileRange), defId(_defId), subProof(std::move(_subProof))
	{
	}
	void print() const override;
	inline ~Proof_RawWrap() override = default;
};

struct Proof_Shove final : Proof {
	shared_ptr<const Proof> left;
	shared_ptr<const Proof> right;
	constexpr Proof_Shove(FileRange _fileRange, shared_ptr<const Proof>&& _left, shared_ptr<const Proof>&& _right):
		Proof(_fileRange), left(std::move(_left)), right(std::move(_right))
	{
	}
	void print() const override;
	inline ~Proof_Shove() override = default;
};

struct Proof_RawShove final : Proof {
	shared_ptr<const Proof> left;
	shared_ptr<const Proof> right;
	constexpr Proof_RawShove(FileRange _fileRange, shared_ptr<const Proof>&& _left, shared_ptr<const Proof>&& _right):
		Proof(_fileRange), left(std::move(_left)), right(std::move(_right))
	{
	}
	void print() const override;
	inline ~Proof_RawShove() override = default;
};


struct Proof_Substitute final : Proof {
	vector<pair<string_view, shared_ptr<const Expression>>> vec;
	shared_ptr<const Proof> subProof;
	constexpr Proof_Substitute(FileRange _fileRange, vector<pair<string_view, shared_ptr<const Expression>>>&& _vec, shared_ptr<const Proof>&& _subProof):
		Proof(_fileRange), vec(std::move(_vec)), subProof(std::move(_subProof)) { }
	void print() const override;
	inline ~Proof_Substitute() override = default;
};

