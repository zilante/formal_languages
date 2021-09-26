#include <iostream>
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>
#include <stack>
#include <unordered_set>
#include <limits>

struct State {
public:
	State();
	explicit State(bool is_terminal_);
	bool is_terminal;
	std::unordered_multimap<char, std::shared_ptr<State>> children;
	int number;
};

typedef std::shared_ptr<State> state_ptr;

State::State() {
	is_terminal = false;
	number = -1;
}

State::State(bool is_terminal_) {
	is_terminal = is_terminal_;
	number = -1;
}

class NotDeterminateFiniteMachine {
public:
	NotDeterminateFiniteMachine() = default;
	explicit NotDeterminateFiniteMachine(const char symbol);
	explicit NotDeterminateFiniteMachine(const std::string& regular_expression);
	int GetShortestWordLengthWithPrefix(const char prefix_symbol,
		const int prefix_length) const; // prefix = prefix_symbol^prefix_length

	friend std::shared_ptr<NotDeterminateFiniteMachine> GetUnion(
		std::shared_ptr<NotDeterminateFiniteMachine> machine1,
		std::shared_ptr<NotDeterminateFiniteMachine> machine2);

	friend std::shared_ptr<NotDeterminateFiniteMachine> GetConcatenation(
		std::shared_ptr<NotDeterminateFiniteMachine> machine1,
		std::shared_ptr<NotDeterminateFiniteMachine> machine2);

	friend std::shared_ptr<NotDeterminateFiniteMachine> GetIteration(
		std::shared_ptr<NotDeterminateFiniteMachine> machine);

private:
	state_ptr start_state_;
	std::vector<state_ptr> terminals_;
};

typedef std::shared_ptr<NotDeterminateFiniteMachine> machine_ptr;

NotDeterminateFiniteMachine::NotDeterminateFiniteMachine(const char symbol) {
	if (symbol == '1') {
		start_state_ = std::make_shared<State>(true);
		terminals_.push_back(start_state_);
	}
	else if (symbol == 'a' || symbol == 'b' || symbol == 'c') {
		start_state_ = std::make_shared<State>(false);
		auto terminal = std::make_shared<State>(true);
		start_state_->children.insert({ symbol, terminal });
		terminals_.push_back(terminal);
	}
	else {
		start_state_ = nullptr; // not correct regular expression
	}
}

inline bool IsAlphabetSymbol(const char symbol) {
	return symbol == 'a' || symbol == 'b' || symbol == 'c' || symbol == '1';
}

inline bool IsOperator(const char symbol) {
	return symbol == '+' || symbol == '*' || symbol == '.';
}

machine_ptr GetUnion(machine_ptr machine1, machine_ptr machine2) {
	if (machine1 == nullptr || machine2 == nullptr) {
		return nullptr;
	}
	if (machine1->start_state_ == nullptr || machine2->start_state_ == nullptr) {
		return nullptr;
	}

	bool is_start_state_terminal = machine1->start_state_->is_terminal ||
		machine2->start_state_->is_terminal;
	auto start_state = std::make_shared<State>(is_start_state_terminal);
	start_state->children = machine1->start_state_->children;
	for (auto child : machine2->start_state_->children) {
		start_state->children.insert(child);
	}

	auto machine = std::make_shared<NotDeterminateFiniteMachine>();
	machine->start_state_ = start_state;
	machine->terminals_ = machine1->terminals_;
	for (auto terminal : machine2->terminals_) {
		machine->terminals_.push_back(terminal);
	}
	return machine;
}

// warning: spoils arguments
machine_ptr GetConcatenation(machine_ptr machine1, machine_ptr machine2) {
	if (machine1 == nullptr || machine2 == nullptr) {
		return nullptr;
	}
	if (machine1->start_state_ == nullptr || machine2->start_state_ == nullptr) {
		return nullptr;
	}

	auto machine = std::make_shared<NotDeterminateFiniteMachine>();
	machine->start_state_ = machine1->start_state_;
	machine->terminals_ = machine2->terminals_;

	for (auto machine1_terminal : machine1->terminals_) {
		machine1_terminal->is_terminal = machine2->start_state_->is_terminal;
		if (machine1_terminal->is_terminal) {
			machine->terminals_.push_back(machine1_terminal);
		}
		for (auto child : machine2->start_state_->children) {
			machine1_terminal->children.insert(child);
		}
	}
	return machine;
}

//spoils argument
machine_ptr GetIteration(machine_ptr machine) {
	if (machine == nullptr) {
		return nullptr;
	}
	if (machine->start_state_ == nullptr) {
		return nullptr;
	}

	auto start_state = std::make_shared<State>(true);
	for (auto child : machine->start_state_->children) {
		start_state->children.insert(child);
		for (auto terminal : machine->terminals_) {
			terminal->children.insert(child);
		}
	}
	machine->start_state_ = start_state;
	machine->terminals_.push_back(start_state);
	return machine;
}

machine_ptr GetOperationResult(const char operation,
	std::stack<machine_ptr>& intermediate_machines) {
	if (operation == '+') {
		if (intermediate_machines.size() < 2) {
			return nullptr;
		}
		auto machine2 = intermediate_machines.top();
		intermediate_machines.pop();
		auto machine1 = intermediate_machines.top();
		intermediate_machines.pop();
		return GetUnion(machine1, machine2);
	}
	else if (operation == '.') {
		if (intermediate_machines.size() < 2) {
			return nullptr;
		}
		auto machine2 = intermediate_machines.top();
		intermediate_machines.pop();
		auto machine1 = intermediate_machines.top();
		intermediate_machines.pop();
		return GetConcatenation(machine1, machine2);
	}
	else if (operation == '*') {
		if (intermediate_machines.size() < 1) {
			return nullptr;
		}
		auto machine = intermediate_machines.top();
		intermediate_machines.pop();
		return GetIteration(machine);
	}
}

NotDeterminateFiniteMachine::NotDeterminateFiniteMachine(
	 const std::string& regular_expression) {
	std::stack<machine_ptr> intermediate_machines;
	start_state_ = nullptr;
	int state_count = 0;

	if (regular_expression.size() == 0) {
		return; // regular expression is not correct
	}

	for (auto symbol : regular_expression) {
		machine_ptr machine = nullptr;

		if (IsAlphabetSymbol(symbol)) {
			machine = std::make_shared<NotDeterminateFiniteMachine>(symbol);
			machine->start_state_->number = state_count++;
			for (auto child : machine->start_state_->children) {
				child.second->number = state_count++;
			}
		}
		else if (IsOperator(symbol)) {
			machine = GetOperationResult(symbol, intermediate_machines);
			if (machine == nullptr) {
				return;
			}
			if (machine->start_state_->number == -1) {
				machine->start_state_->number = state_count++;
			}
		}

		if (machine == nullptr) {
			return;
		}
		intermediate_machines.push(machine);
	}
	if (intermediate_machines.size() != 1) {
		return;
	}
	else {
		start_state_ = intermediate_machines.top()->start_state_;
		terminals_ = intermediate_machines.top()->terminals_;
	}
}

int NotDeterminateFiniteMachine::GetShortestWordLengthWithPrefix(
	const char prefix_symbol, const int prefix_length) const {
	if (start_state_ == nullptr) {
		return -1; // regular expression is not correct
	}
	std::vector<state_ptr> current_states;
	std::vector<state_ptr> new_states;
	current_states.push_back(start_state_);
	std::unordered_set<int> new_state_numbers;
	int answer = 0;

	for (int i = 0; i < prefix_length; ++i) {
		for (auto state : current_states) {
			auto iterator_pair = state->children.equal_range(prefix_symbol);
			for (auto it = iterator_pair.first; it != iterator_pair.second; ++it) {
				if (new_state_numbers.count(it->second->number) == 0) {
					new_states.push_back(it->second);
					new_state_numbers.insert(it->second->number);
				}
			}
		}
		if (new_states.size() == 0) {
			answer = std::numeric_limits<int>::max();
			return answer;
		}
		current_states = new_states;
		new_states.clear();
		new_state_numbers.clear();
	}
	answer = prefix_length;
	for (auto state : current_states) {
		if (state->is_terminal) {
			return answer;
		}
	}
	while (true) {
		answer++;
		for (auto state : current_states) {
			for (auto child : state->children) {
				if (new_state_numbers.count(child.second->number) == 0) {
					if (child.second->is_terminal) {
						return answer;
					}
					new_states.push_back(child.second);
					new_state_numbers.insert(child.second->number);
				}
			}
		}
		current_states = new_states;
		new_states.clear();
		new_state_numbers.clear();
	}
}

int main() {
	std::string regular_expression = "";
	char prefix_symbol = 'a';
	int prefix_length = 0;
	std::cin >> regular_expression >> prefix_symbol >> prefix_length;
	auto machine = std::make_shared<NotDeterminateFiniteMachine>(regular_expression);
	int shortest_word_length_with_prefix = machine->GetShortestWordLengthWithPrefix(prefix_symbol, prefix_length);
	if (shortest_word_length_with_prefix == -1) {
		std::cout << "ERROR";
	}
	else if (shortest_word_length_with_prefix == std::numeric_limits<int>::max()) {
		std::cout << "INF";
	}
	else {
		std::cout << shortest_word_length_with_prefix;
	}
	return 0;
}