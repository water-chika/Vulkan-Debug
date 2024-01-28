#include <map>
#include <iostream>
#include <chrono>
#include <functional>

class fun {
public:
	virtual int operator()(int n) {
		if (n <= 0) {
			return 1;
		}
		return operator()(n - 1) + operator()(n - 2);
	}
};

class fun_cached : fun {
public:
	int operator()(int n) override {
		int res;
		if (cache.find(n) != cache.end()) {
			res = cache[n];
		}
		else {
			res = fun::operator()(n);
			cache.emplace(n, res);
		}
		return res;
	}
private:
	std::map<int, int> cache;
};


template<class F, class... Args>
class cached_call : public F {
public:
	using index_type = std::tuple<Args...>;
	auto operator()(Args... args) -> decltype(F::operator()(args)) override {
		index_type index{ args };
		ret_type res{};
		if (cache.find(index) != cache.end()) {
			res = cache[index];
		}
		else {
			res = F::operator()(index);
			cache.emplace(index, res);
		}
		return res;
	}
private:
	std::map<index_type, ret_type> cache;
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
	std::cout << measure_duration(fun{}, 30) << std::endl;
	std::cout << measure_duration(cached_call<fun, int>{}, 30) << std::endl;
	return 0;
}