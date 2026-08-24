export module BusLang:FileRange;

import std;

export struct FilePos {
	unsigned int index;
	unsigned int line;
	unsigned int col;
};

export struct FileRange {
	FilePos start;
	unsigned int length;
	constexpr FileRange(FilePos _start, unsigned int _length): start(_start), length(_length) { }
	
	constexpr static FileRange firstLast(FilePos a, FilePos b) noexcept {
		if (a.index > b.index) std::swap(a, b);
		return FileRange { a, b.index - a.index + 1 };
	}
	constexpr static FileRange startEnd(FilePos a, FilePos b) {
		if (a.index > b.index) throw 1932112;
		return FileRange { a, b.index - a.index };
	}
	static constexpr FileRange none() noexcept {
		return FileRange(FilePos{0, 0, 0}, 0);
	}
	
	constexpr unsigned int startIndex() const noexcept { return start.index; }
	constexpr unsigned int lastIndex() const noexcept { return start.index + length - 1; }
	
	static constexpr FileRange span(const FileRange& a, const FileRange& b) noexcept {
		if (a.length == 0) return b;
		if (b.length == 0) return a;
		const FilePos& first = a.startIndex() < b.startIndex() ? a.start : b.start;
		const unsigned int lastIndex = std::max(a.startIndex() + a.length, b.startIndex() + b.length) - 1;
		return FileRange(
			first,
			lastIndex - first.index
		);
	}
};
