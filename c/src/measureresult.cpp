#include <tirex_tracker.h>

#include "measure/stats/provider.hpp"
#include "measure/utils/rangeutils.hpp"

#if __cpp_lib_format
#include <format>
namespace _fmt = std;
#else
#include <fmt/core.h>
namespace _fmt = fmt;
#endif

#include <iostream>

struct tirexResult_st {
public:
	std::vector<std::pair<tirexMeasure, std::string>> value;

public:
	explicit tirexResult_st(std::vector<std::pair<tirexMeasure, std::string>>&& val) : value(std::move(val)) {}
};

tirexError tirexResultEntryGetByIndex(const tirexResult* result, size_t index, tirexResultEntry* entry) {
	if (result == nullptr || index >= result->value.size())
		return tirexError::TIREX_INVALID_ARGUMENT;
	const auto& [source, value] = result->value.at(index);
	*entry = {.source = source, .value = value.c_str(), .type = tirexResultType::TIREX_STRING};
	return tirexError::TIREX_SUCCESS;
}

tirexError tirexResultEntryNum(const tirexResult* result, size_t* num) {
	if (result == nullptr)
		return tirexError::TIREX_INVALID_ARGUMENT;
	*num = result->value.size();
	return tirexError::TIREX_SUCCESS;
}

void tirexResultFree(tirexResult* result) { delete result; }

template <class... Ts>
struct overloaded : Ts... {
	using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

template <typename T>
static std::string toYAML(const tirex::TimeSeries<T>& timeseries) {
	const auto& [timestamps, values] = timeseries.timeseries();
	return _fmt::format(
			"{{max: {}, min: {}, avg: {}, timeseries: {{timestamps: [{}], values: [{}]}}}}", timeseries.maxValue(),
			timeseries.minValue(), timeseries.avgValue(), tirex::utils::join(timestamps, ','),
			tirex::utils::join(values, ',')
	);
}

extern tirexResult_st* tirex::createMsrResultFromStats(tirex::Stats&& stats) {
	std::vector<std::pair<tirexMeasure, std::string>> result;
	for (auto&& [key, value] : stats) {
		std::visit(
				overloaded{
						[key, &result](std::string& str) { result.emplace_back(key, std::move(str)); },
						[key, &result](const tirex::TimeSeries<unsigned>& timeseries) {
							result.emplace_back(key, toYAML(timeseries));
						}
				},
				value
		);
	}
	return new tirexResult_st(std::move(result));
}