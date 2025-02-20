#ifndef STATS_GITSTATS_HPP
#define STATS_GITSTATS_HPP

#include "provider.hpp"

struct git_repository;

namespace msr {
	class GitStats final : public StatsProvider {
	private:
		git_repository* repo;

	public:
		GitStats();
		~GitStats();

		bool isRepository() const noexcept;

		void start() override;
		void stop() override;
		Stats getInfo() override;

		static constexpr const char* description = "Collects git related metrics.";
		static const char* version;
		static const std::set<msrMeasure> measures;
	};
} // namespace msr

#endif