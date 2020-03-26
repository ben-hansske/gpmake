#include <core/queue.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <sstream>

<<<<<<< HEAD
=======
static int glob_counter = 0;
>>>>>>> cpp
struct element_t{
	static unsigned counter;
	element_t() {
		dummy_data = counter;
		++counter;
	}
	unsigned dummy_data;
};
unsigned element_t::counter = 0;

struct test_val{
	static int counter;
	test_val() {
		counter++;
		std::cout << "def_cstor " << counter << std::endl;
	}
	~test_val() {
		counter--;
		std::cout << "def_dstor " << counter << std::endl;
	}
	test_val(test_val &&) {
		counter++;
		std::cout << "mov_cstor " << counter << std::endl;
	}
	int data = 0; 
};

int test_val::counter = 0;

void d_assert(bool cond, std::string val = "unknown assertion") {
	if(!cond) {
		throw std::runtime_error(std::move(val));
	}
}


template<typename ...Args>
[[nodiscard]] auto mk_string(Args&&... args) -> std::string {
	auto os = std::ostringstream();
	(os << ... << std::forward<Args>(args));
	return os.str();
}

int main() {
	{
		auto queue2 = queue<test_val>(4);
		for(int i = 0; i < 10; ++i) {
			queue2.push();
			d_assert(test_val::counter == i + 1,mk_string(__LINE__));
		}
		for(int i = 0; i < 5; ++i) {
			auto x = queue2.pop();
			d_assert(test_val::counter == 10 - i, mk_string(__LINE__, " -> ", i, "Expected: ", 10 - i, " counted: ", test_val::counter));
		}
		for (int i = 0; i < 3; ++i) {
			auto x = queue2.pop();
			d_assert(test_val::counter == 5, mk_string(__LINE__));
			queue2.push(std::move(x));
			d_assert(test_val::counter == 6,mk_string(__LINE__));
		}
		d_assert(test_val::counter == 5,mk_string(__LINE__));
	}
	d_assert(test_val::counter == 0, mk_string(__LINE__, ": Expected: 0 Counted: ", test_val::counter));

	{
		auto q = queue<std::unique_ptr<element_t>>(10);

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

}
