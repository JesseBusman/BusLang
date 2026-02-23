#pragma once

#include <string_view>

extern unsigned int idCounter;

struct Id {
	std::string_view name;
	unsigned int id;
	constexpr Id() noexcept: name(), id(0) { }
private:
	constexpr Id(std::string_view _name, unsigned int _id) noexcept: name(_name), id(_id) { }
	
public:
	[[nodiscard]] static inline Id make(std::string_view name) noexcept {
		idCounter++;
		return Id(name, idCounter);
	}
	[[nodiscard]] constexpr bool operator <  (const Id& other) const noexcept { return id <  other.id; }
	[[nodiscard]] constexpr bool operator >  (const Id& other) const noexcept { return id >  other.id; }
	[[nodiscard]] constexpr bool operator <= (const Id& other) const noexcept { return id <= other.id; }
	[[nodiscard]] constexpr bool operator >= (const Id& other) const noexcept { return id >= other.id; }
	[[nodiscard]] constexpr bool operator == (const Id& other) const noexcept { return id == other.id; }
	[[nodiscard]] constexpr bool operator != (const Id& other) const noexcept { return id != other.id; }
};
