#include<core/queue.hpp>
#include<iostream>

unsigned glob_counter = 0;
struct element{
	element() {
		dummy_data = glob_counter;
		++glob_counter;
	}
	int dummy_data;
};

int main() {
	queue<std::unique_ptr<element>> q(10);
	
	for(unsigned i = 0; i < 8; ++i) {
		q.push();
	}
	auto res = q.pop();
	for(unsigned i = 0; i < 3; ++i) {
		q.push();
	}
	q.push(std::move(res));
	for(unsigned i = 0; i < 12; ++i) {
		q.push();
	}
	while(!q.is_empty()) {
		auto ptr = q.pop();
	}
}
