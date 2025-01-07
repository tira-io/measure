#ifndef CONFIG_HPP
#define CONFIG_HPP

#include "logging.hpp"
#include "statformatter.hpp"
#include "stats/provider.hpp"

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <vector>

namespace am {
	struct LoggerConf final {
		bool quiet = false;
		/**
         * @brief Sets the verbosity of the logger. Per default (verbosity=0) only critical, error and warnings are printed.
         * If verbosity is...
         *  - 1: info and above is printed
         *  - 2: debug and above is printed
         *  - 3+: trace and above is printed
         */
		int verbosity = 0;

		am::Verbosity getVerbosity() const noexcept {
			if (quiet)
				return am::Verbosity::Off;
			auto verb = static_cast<int>(am::Verbosity::Warning) + verbosity;
			verb = std::min(verb, static_cast<int>(am::Verbosity::Trace));
			return static_cast<am::Verbosity>(verb);
		}
	};

	struct MeasureCmdArgs final {
		LoggerConf logConf;
		std::string command;   /**< The command that should be measured **/
		std::string formatter; /**< The identifier specifying the formatter to use for the output **/
		std::vector<std::string> statproviders;
		bool monitor = true;
		size_t pollIntervallMs;
		bool pedantic;

		std::unique_ptr<StatFormatter> constructFormatter(const am::Stats& stats) const;
		std::vector<std::unique_ptr<am::StatsProvider>> constructProviders() const;
	};

	static void setupLoggerArgs(CLI::App& app, LoggerConf& conf) {
		app.add_flag(
				"-v,--verbose", conf.verbosity,
				"Sets the logger's verbosity. Passing it multiple times increases verbosity."
		);
		app.add_flag("-q,--quiet", conf.quiet, "Supresses all outputs");
	}
} // namespace am

#endif