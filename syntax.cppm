export module BusLang:Syntax;

import std;
import :Id;
import :Expression;

export {

enum Associativity {
	LEFT,
	RIGHT,
	NOASSOC,
};

struct Space { };

struct Syntax {
	std::map<
		Id,
		std::vector<std::pair<
			std::vector<std::variant<Id, char, Space, std::pair<Id, Id>>>,
			std::shared_ptr<const Expression>
		>>
	> name_to_syntaxAndExpr;
	std::vector<std::variant<Id, char, Space, std::pair<Id, Id>>> syntax;
	std::shared_ptr<const Expression> expr;
	
	std::optional<long> precedence;
	Associativity associativity;
};

}
