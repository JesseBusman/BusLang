export module BusLang:SyntaxError;

import std;
import :FileRange;

export struct SyntaxError {
	std::string message;
	FileRange fileRange1;
	std::optional<FileRange> fileRange2;
	
	SyntaxError(auto&& _message, FilePos _filePos):
		message(_message), fileRange1(_filePos, 1), fileRange2(std::nullopt) { }
	
	SyntaxError(auto&& _message, FilePos _filePos, FilePos _filePos2):
		message(_message), fileRange1(_filePos, 1), fileRange2(FileRange(_filePos2, 1)) { }
	
	SyntaxError(auto&& _message, FileRange _fileRange):
		message(_message), fileRange1(_fileRange), fileRange2(std::nullopt) { }
	
	SyntaxError(auto&& _message, FileRange _fileRange, FileRange _fileRange2):
		message(_message), fileRange1(_fileRange), fileRange2(_fileRange2) { }
};
