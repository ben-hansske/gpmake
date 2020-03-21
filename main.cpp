extern "C" {
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#include <iostream>
#include <string>
#include <map>
#include <shared_mutex>
#include <vector>
#include <stdexcept>
#include <sstream>

int add(lua_State * L)
{
	int num_args = lua_gettop(L);
	int i;
	double sum = 0;


	for (i = 1; i <= num_args; i++) {
		sum += lua_tonumber(L, i);
	}

	lua_pushnumber(L, sum);	// need to push the result back on the stack

	return lua_yield(L, 1); // 1 result
}

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
		default:
			throw std::runtime_error(mk_string("Unknown dependency_type: ", static_cast<int>(val)));
	}
}


struct DTable
{
	using dep_table_t = std::map<std::string, dependency_type>;
	using dep_it_t = dep_table_t::iterator;
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

struct LBuildLib
{
	[[nodiscard]] static auto self(lua_State * state) {
		return reinterpret_cast<LBuildLib*>(lua_touserdata(state, lua_upvalueindex(1)));
	}

	[[nodiscard]] auto require_dependencies(lua_State * state) {
		auto num_args = lua_gettop(state);
		std::cout << this << "::require_dependency called:\n";
		for(int i = 1; i <= num_args; ++i) {
			std::cout << lua_tostring(state, i) << std::endl;
		}

		return 0;
	}
	[[nodiscard]] auto changed_dependencies(lua_State * state) {
		return 0;
	}
	[[nodiscard]] auto satisfy_dependencies(lua_State * state) {
		return 0;
	}
	[[nodiscard]] auto failed_dependencies(lua_State * state) {
		return 0;
	}
	[[nodiscard]] auto spawn_task(lua_State * state) {
		return 0;
	}

	[[nodiscard]] static int lua_require_dependencies(lua_State * state) { return self(state)->require_dependencies(state); }
	[[nodiscard]] static int lua_satisfy_dependencies(lua_State * state) { return self(state)->satisfy_dependencies(state); }
	[[nodiscard]] static int lua_changed_dependencies(lua_State * state) { return self(state)->changed_dependencies(state); }
	[[nodiscard]] static int lua_failed_dependencies(lua_State * state) { return self(state)->failed_dependencies(state); }
	[[nodiscard]] static int lua_spawn_task(lua_State * state) { return self(state)->spawn_task(state); }
};

struct LTask
{
private:
	lua_State * m_state;
	lua_State * m_thread;
	std::string m_code;
	LBuildLib m_lbuild;
public:
	LTask():
		m_state(luaL_newstate()),
		m_thread(lua_newthread(m_state))
	{
		luaL_openlibs(m_state);
		const luaL_Reg reg[]{
			{"require_dependencies", &LBuildLib::lua_require_dependencies},
			{"satisfy_dependencies", &LBuildLib::lua_satisfy_dependencies},
			{"failed_dependencies",  &LBuildLib::lua_failed_dependencies},
			{"changed_dependencies", &LBuildLib::lua_changed_dependencies},
			{"spawn_task", &LBuildLib::lua_spawn_task},
			{nullptr, nullptr}
		};

		lua_newtable(m_state);
		lua_pushlightuserdata(m_state, &m_lbuild);
		luaL_setfuncs(m_state, reg, 1);
		lua_pushvalue(m_state, -1);
		lua_setglobal(m_state, "lbuild");
	}
	auto get_state() { return m_state; }
	explicit LTask(const std::string& code): LTask() {
		m_code = code;
		std::cout << m_code << std::endl;
		luaL_loadstring(m_thread, m_code.c_str());
	}
	~LTask() {
		//lua_close(m_thread);
		lua_close(m_state);
	}
	std::string state_error() {
		return lua_tostring(m_state, -1);
	}
	std::string thread_error() {
		return lua_tostring(m_thread, -1);
	}
	int resume() {
		return lua_resume(m_thread, m_state, 0);
	}


};


int main(int argc, char**argv)
{
	auto task = LTask(R"lua(
		lbuild.require_dependencies("test");
		)lua");

	std::cout << &task <<std::endl;
	auto task_res = task.resume();
	std::cout << task_res << std::endl;
	if(task_res == LUA_OK) {
		std::cout << "finished!\n";
	}
	else if (task_res == LUA_YIELD) {
		std::cout << "yielded!\n";
	}
	else {
		std::cout << "ERROR: " << task.thread_error();// << '\n' << task.thread_error();
	}

};
