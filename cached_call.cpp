#include <map>
#include <iostream>
#include <chrono>
#include <functional>

template<class F, class... Args>
class cached_caller {
	using ret_type = decltype(fun(args...));
	using index_type = std::tuple<Args...>;

	ret_type operator()(Args... args) {
		ret_type res;
		index_type index{ args... };
		if (m_cache.count(index) > 0) {
			res = m_cache[index];
		}
		else {
			res = fun(args...);
			m_cache.emplace(index, res);
		}
		return res;
	}
private:
	static std::map<index_type, ret_type> m_cache;
};


template<class F, class... Args>
requires std::invocable<F, Args...>
auto measure_duration(F f, Args... args) {
	auto begin = std::chrono::steady_clock::now();
	auto res = f(args...);
	auto end = std::chrono::steady_clock::now();
	auto duration = end - begin;
	std::cout << duration << std::endl;
	return res;
}

int main() {
	return 0;
}