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
#include <core/queue.hpp>

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
			m_deps[dep] = val;
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
	auto read(dep_it_t it) {
		auto lck = get_shared_lock();
		return *it;
	}

	std::shared_mutex m_mtx;
	[[nodiscard]] auto require_dependencies(std::string dep) {
		auto lck = std::unique_lock(m_mtx);
		auto it = m_deps.emplace(dep, dependency_type::not_found).first;
		return it;
	}
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

	[[nodiscard]] auto satisfy_dependency(const std::string& dep) {
		auto lck = std::unique_lock(m_mtx);
		insert_dependency(dep, dependency_type::satisfied);
	}
	[[nodiscard]] auto satisfy_dependencies(const std::vector<std::string>& deps) {
		auto lck = std::unique_lock(m_mtx);
		for(auto & dep : deps) {
			insert_dependency(dep, dependency_type::satisfied);
		}
	}
	[[nodiscard]] auto changed_dependency(const std::string& dep) {
		auto lck = std::unique_lock(m_mtx);
		insert_dependency(dep, dependency_type::changed);
	}
	[[nodiscard]] auto changed_dependencies(const dep_vec & deps) {
		auto lck = std::unique_lock(m_mtx);
		for(auto & dep: deps) {
			insert_dependency(dep, dependency_type::changed);
		}
	}
	[[nodiscard]] auto failed_dependency(const std::string& dep){
		auto lck = std::unique_lock(m_mtx);
		insert_dependency(dep, dependency_type::failed);
	}
	[[nodiscard]] auto failed_dependencies(const dep_vec & deps) {
		auto lck = std::unique_lock(m_mtx);
		for(auto & dep: deps) {
			insert_dependency(dep, dependency_type::failed);
		}
	}

};

template <typename T>
using dring_queue = queue<T>;

struct warning_exception: std::runtime_error { using std::runtime_error::runtime_error; };
struct failure_exception: std::runtime_error { using std::runtime_error::runtime_error; };
struct critical_exception: std::runtime_error { using std::runtime_error::runtime_error; };

enum class task_return_type {
	success,
	dependencies_missing,
};


template<typename Fun>
class on_scope_exit{
	Fun m_fun;
public:
	on_scope_exit(Fun && fun): m_fun(std::move(fun)){}
	on_scope_exit(const Fun& fun): m_fun(fun){}
	~on_scope_exit(){m_fun();}
};
template<typename Fun>
on_scope_exit(Fun &&) -> on_scope_exit<Fun>;
template<typename Fun>
on_scope_exit(const Fun&) -> on_scope_exit<Fun>;



class context
{
	using task_t = std::function<task_return_type(context&)>;
	using task_ptr = std::unique_ptr<task_t>;

	std::condition_variable cv;
	dring_queue<task_ptr> m_tasks;
	mutable std::mutex m_mtx;

	std::vector<std::thread> m_threads;
	std::vector<std::string> m_failed;
	std::vector<std::string> m_warnings;
	std::optional<std::string> m_critical;

	std::size_t working_threads = 0;
	std::size_t tasks_to_try = 0;
	std::size_t task_queue_size = 0; //TODO make it a function of queue

	bool m_stop = false;
private:
	struct lock_ref{
		lock_ref(const std::lock_guard<std::mutex>&){}
		lock_ref(const std::unique_lock<std::mutex>&){}
		lock_ref(const std::lock_guard<std::mutex>&&){}
		lock_ref(const std::unique_lock<std::mutex>&&){}
	};
	void task_succeeded(){
		auto lck = std::lock_guard(m_mtx);
		task_succeeded(lck);
	}
	void task_succeeded(lock_ref) {
		tasks_to_try = task_queue_size + working_threads;
		cv.notify_all();
	}
	void stop_threads(lock_ref) {
		m_stop = true;
		cv.notify_all();
	}

	void wake_threads() {
		auto lck = std::unique_lock(m_mtx);
		wake_threads(lck);
	}

	void wake_threads(lock_ref) {
		cv.notify_all();
	}

	void respawn_task(task_ptr&& task) {
		auto lck = std::unique_lock(m_mtx);
		m_tasks.push(std::move(task));
		task_queue_size++;
		//tasks_to_try = task_queue_size;
	}
public:

	context():m_tasks(100) {
	}
	void launch(std::size_t num_threads = std::thread::hardware_concurrency() - 1) {
		m_threads.reserve(num_threads);
		for(std::size_t i = 0; i < num_threads; ++i) {
			m_threads.emplace_back(&context::run_thread, this);
		}
	}

	void stop_threads(){
		auto lck = std::lock_guard(m_mtx);
		stop_threads(lck);
	}
	void wait_finished() {
		run_thread();
	}
	~context() {
		stop_threads();
		for(auto & t : m_threads) {
			t.join();
		}
	}

	template<typename Task>
	void spawn_task(Task&& task) {
		auto lck = std::unique_lock(m_mtx);
		if constexpr (std::is_same_v<Task, task_ptr>) {
			m_tasks.push(std::move(task));
		}
		else {
			m_tasks.push(std::make_unique<task_t>(std::forward<Task>(task)));
		}
		task_queue_size++;
		tasks_to_try = task_queue_size;
	}




	void run_thread() {
		{
			auto lck = std::unique_lock(m_mtx);
			working_threads++;
		}
		auto rel_work_thr = on_scope_exit([&]{
			auto lck = std::unique_lock(m_mtx);
			working_threads--; }
		);
		for(;;) {
			//std::cout << "t\n";
			auto tsk = [&]() -> task_ptr{
				auto lck = std::unique_lock(m_mtx);
				--working_threads;
				if(working_threads == 0 && tasks_to_try == 0) {
					m_stop = true;
					cv.notify_all();
					return nullptr;
				}

				std::cout << tasks_to_try << std::endl;
				cv.wait(lck, [&]{ return (!m_tasks.is_empty() && tasks_to_try > 0) || m_stop; });
				working_threads ++;
				if(m_stop) { return nullptr; }
				--task_queue_size;
				--tasks_to_try;
				return m_tasks.pop();
			}();

			if (!tsk) {
				std::cout << "t2\n";
				return; }
			try {
				switch ((*tsk)(*this)) {
					case task_return_type::success:
						task_succeeded();
						break;
					case task_return_type::dependencies_missing:
						respawn_task(std::move(tsk));
						break;
					default:
						throw std::runtime_error("wrong enum value");
				}
			}
			catch (warning_exception& e) {
				auto lck = std::lock_guard(m_mtx);
				m_warnings.push_back(e.what());
				task_succeeded(lck);
			}
			catch (failure_exception& e) {
				auto lck = std::lock_guard(m_mtx);
				m_failed.push_back(e.what());
				task_succeeded(lck);
			}
			catch (critical_exception& e) {
				auto lck = std::lock_guard(m_mtx);
				m_critical = std::string(e.what());
				stop_threads(lck);
			}
		}
	}

	const auto& failed() const {
		auto lck = std::lock_guard(m_mtx);
		return m_failed;
	}
	const auto& warnings() const {
		auto lck = std::lock_guard(m_mtx);
		return m_warnings;
	}
	const auto& critical() const {
		auto lck = std::lock_guard(m_mtx);
		return m_critical;
	}

};

int main()
{
	context ctx;
	dependency_table dtable;
	for(int i = 0; i < 100; ++i) {
		auto fun = [&dtable, i](context&){
			auto result = dtable.require_dependencies(mk_string("start",i+1));
			switch(dtable.read(result).second) {
				case dependency_type::not_found:
					//std::cout << mk_string(i, " missing dep\n");
					return task_return_type::dependencies_missing;
				case dependency_type::changed:
					std::cout << mk_string(i, " changed dep\n");
					dtable.changed_dependency(mk_string("start", i+1));
					throw warning_exception("uhhh my dependency changed... there will be a lot of work");
				case dependency_type::satisfied:
					std::cout << mk_string("Hallo ", i, '\n');
					dtable.satisfy_dependency(mk_string("start", i));
					return task_return_type::success;
				case dependency_type::failed:
					std::cout << mk_string(i, " failed dep\n");
					dtable.failed_dependency(mk_string("start", i));
					throw failure_exception("my dependencies couldn't be satisfied");
			}
		};
		ctx.spawn_task(fun);
	}
	ctx.spawn_task([&dtable](context&){
		dtable.failed_dependency("start100");
		return task_return_type::success;
		});
	ctx.launch();
	ctx.wait_finished();
	for(auto & e : ctx.failed()) {
		std::cout << e << std::endl;
	}
	for(auto & e : ctx.warnings()) {
		std::cout << e << std::endl;
	}
	if(ctx.critical()) {
		std::cout << *ctx.critical() << std::endl;
	}
	std::cout << "--------------------end--------------------\n";
}
