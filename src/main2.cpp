#include <functional>
#include <memory>
#include <atomic>
#include <condition_variable>
#include <thread>
#include <shared_mutex>
#include <sstream>
#include <map>
#include <string>
#include <optional>
#include <iostream>

template <typename ...Args>
[[nodiscard]] auto mk_string(Args && ... args) -> std::string {
	auto os = std::ostringstream();
	(os << ... << args);
	return os.str();
}

enum class dependency_type {
	not_found,
	changed,
	satisfied,
	failed
};

auto operator << (std::ostream& os, dependency_type val) -> std::ostream& {
	switch (val) {
		case dependency_type::not_found:
			return os << "not_found";
		case dependency_type::changed:
			return os << "changed";
		case dependency_type::satisfied:
			return os << "satisfied";
		case dependency_type::failed:
			return os << "failed";
	}
	throw std::runtime_error(mk_string("Unknown dependency_type with intergral representation", static_cast<int>(val), '\n'));
}

struct dependency_table
{
	using dep_table_t = std::map<std::string, dependency_type>;
	using dep_it_t = dep_table_t::const_iterator;
	dep_table_t m_deps;
	using dep_vec = std::vector<std::string>;
private:
	void insert_dependency(const std::string& dep, dependency_type val) {
		auto it = m_deps.emplace(dep, dependency_type::not_found).first;
		if(it->second == dependency_type::not_found) {
			m_deps[dep] = dependency_type::satisfied;
		}
		else {
			throw std::runtime_error(mk_string(
						"can't insert dependency again:\n\tname: ", dep,
						"\n\tprevious: ", it->second,
						"\n\tnew: ", val, '\n'));
		}
	}


public:
	auto get_shared_lock() {
		return std::shared_lock(m_mtx);
	}

	std::shared_mutex m_mtx;
	[[nodiscard]] auto require_dependencies(const std::vector<std::string>& deps) {
		auto lck = std::unique_lock(m_mtx);
		auto its = std::vector<dep_it_t>();
		its.reserve(deps.size());
		for(auto & dep: deps) {
			auto it = m_deps.emplace(dep, dependency_type::not_found).first;
			its.push_back(it);
		}
		return its;
	}

	[[nodiscard]] auto satisfy_dependencies(const std::vector<std::string>& deps) {
		auto lck = std::unique_lock(m_mtx);
		for(auto & dep : deps) {
			insert_dependency(dep, dependency_type::satisfied);
		}
	}
	[[nodiscard]] auto changed_dependencies(const dep_vec & deps) {
		auto lck = std::unique_lock(m_mtx);
		for(auto & dep: deps) {
			insert_dependency(dep, dependency_type::changed);
		}
	}
	[[nodiscard]] auto failed_dependencies(const dep_vec & deps) {
		auto lck = std::unique_lock(m_mtx);
		for(auto & dep: deps) {
			insert_dependency(dep, dependency_type::failed);
		}
	}

};

template <typename T>
class dring_queue{
	public:
	template<typename ... Args>
	void push(Args && ...){}
	T pop(){return T();}
	bool is_empty(){return false;}
};

struct warning_exception: std::runtime_error {};
struct failure_exception: std::runtime_error {};
struct critical_exception: std::runtime_error {};

enum class task_return_type {
	success,
	dependencies_missing,
};



struct context
{
	using task = std::function<task_return_type(context&)>;
	using task_ptr = std::unique_ptr<task>;

	std::condition_variable cv; 
	dring_queue<task_ptr> m_tasks;
	std::mutex m_mtx;
	
	std::vector<std::thread> m_threads;
	std::vector<std::string> failed;
	std::vector<std::string> warnings;
	std::optional<std::string> m_critical;

	std::size_t waiting_threads;

	bool m_stop;

	context(std::size_t num_threads = std::thread::hardware_concurrency() - 1) {
		m_threads.reserve(num_threads);
		for(std::size_t i = 0; i < num_threads; ++i) {
			m_threads.emplace_back(&context::run_thread, this);
		}
	}
	void stop_threads(){
		auto lck = std::lock_guard(m_mtx);
		m_stop = true;
		cv.notify_all();
	}
	~context() {
		stop_threads();
		for(auto & t : m_threads) {
			t.join();
		}
	}



	void run_thread() {
		for(;;) {
			auto tsk = [&]() -> task_ptr{
				auto lck = std::unique_lock(m_mtx);
				waiting_threads++;
				if(waiting_threads >= m_threads.size()) {
					m_stop = true;
					cv.notify_all();
					return nullptr;
				}

				cv.wait(lck, [&]{ return !m_tasks.is_empty() || m_stop; });
				if(m_stop) { return nullptr; }
				return m_tasks.pop();
			}();

			if (!tsk) { return; }
			try {
				switch ((*tsk)(*this)) {
					case task_return_type::success:
						break;
					case task_return_type::dependencies_missing: {	
							auto lck = std::lock_guard(m_mtx);
							m_tasks.push(std::move(tsk));
						}
						break;
					default:
						throw std::runtime_error("wrong enum value");
				}
			}
			catch (warning_exception& e) {
				auto lck = std::lock_guard(m_mtx);
				warnings.push_back(e.what());
			}
			catch (failure_exception& e) {
				auto lck = std::lock_guard(m_mtx);
				failed.push_back(e.what());
			}
			catch (critical_exception& e) {
				auto lck = std::lock_guard(m_mtx);
				m_critical = std::string(e.what());
				m_stop = true;
				cv.notify_all();
			}
		}
	}

	void wake_threads()
	{
		auto lck = std::unique_lock(m_mtx);
		cv.notify_all();
	}

};

int main()
{

}
