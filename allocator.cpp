#include <cstdint>
#include <memory>
#include <vector>
#include <exception>
#include <set>
#include <algorithm>

template<class UINT>
class balancer {
public:
	using uint = UINT;
	using range = std::pair<uint, uint>;
	void add_value(uint v) {
		added_values.emplace_back(v);
		std::ranges::push_heap(added_values);
	}
	void delete_value(uint v) {
		deleted_values.emplace_back(v);
		std::ranges::push_heap(deleted_values);
	}
private:
	std::vector<uint> added_values;
	std::vector<uint> deleted_values;
	std::vector<range> ranges;
};

template<class UINT, uint32_t N>
class allocator {
public:
	using uint = UINT;
	using range = std::pair<uint, uint>;
	allocator() : allocated_values{}, max_value{ 0 }
	{}
	uint allocate(){
		allocated_values.emplace(next_value);
		return next_value++;
	}
	void deallocate(uint v){
		allocated_values.erase(v);
	}
private:
	std::set<uint> allocated_values;
	uint next_value;
};

int main() {
	allocator<uint32_t, UINT32_MAX> mgr;
	int index = mgr.allocate();

	mgr.deallocate(index);
	return 0;
}