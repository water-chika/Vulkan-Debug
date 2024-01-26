#include <map>
#include <iostream>
#include <chrono>

int f_cached(int n);
int f_uncached(int n);

int f(int n) {
	if (n <= 0) {
		return 1;
	}
	return f(n - 1) + f(n - 2);
}

template<class F>
int f(int n, F f_) {
	if (n <= 0) {
		return 1;
	}
	return f_(n - 1) + f_(n - 2);
}

int f_uncached(int n) {
	if (n <= 0) {
		return 1;
	}
	return f_cached(n - 1) + f_cached(n - 2);
}

int f_cached(int n) {
	static std::map<int, int> cache;
	int res;
	if (cache.count(n) > 0) {
		res = cache[n];
	}
	else {
		res = f_uncached(n);
		cache.emplace(n, res);
	}
	return res;
}

template<class F, class... Args>
auto cached_call(F fun, Args... args) -> decltype(fun(args...)){
	using ret_type = decltype(fun(args...));
	using index_type = std::tuple<Args...>;
	static std::map<index_type, ret_type> cache;
	ret_type res;
	index_type index{ args... };
	if (cache.count(index) > 0) {
		res = cache[index];
	}
	else {
		res = fun(args...);
		cache.emplace(index, res);
	}
	return res;
}

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
	std::cout << measure_duration(f, 100) << std::endl;
	std::cout << measure_duration(f_uncached, 100) << std::endl;
	std::cout << measure_duration(f_cached, 100) << std::endl;
	return 0;
}