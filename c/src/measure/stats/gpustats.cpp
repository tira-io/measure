#include "gpustats.hpp"

#include "../../logging.hpp"
#include "../utils/sharedlib.hpp"

#include <nvml/nvml.h>

using namespace std::literals;

using msr::GPUStats;
using msr::Stats;

const char* GPUStats::version = "nvml v." NVML_API_VERSION_STR;
const std::set<msrMeasure> GPUStats::measures{MSR_GPU_SUPPORTED,		   MSR_GPU_MODEL_NAME,
											  MSR_GPU_NUM_CORES,		   MSR_GPU_USED_PROCESS_PERCENT,
											  MSR_GPU_USED_SYSTEM_PERCENT, MSR_GPU_VRAM_USED_PROCESS_MB,
											  MSR_GPU_VRAM_USED_SYSTEM_MB, MSR_GPU_VRAM_AVAILABLE_SYSTEM_MB};

struct NVMLLib final : msr::utils::SharedLib {
public:
	using INIT_V2 = nvmlReturn_t (*)(void);
	INIT_V2 init = load<INIT_V2>({"nvmlInit_v2"});

	using ERROR_STRING = const char* (*)(nvmlReturn_t result);
	ERROR_STRING errorString = load<ERROR_STRING>({"nvmlErrorString"});

	/** SYSTEM */
	using SYSTEM_GET_DRIVER_VERSION = nvmlReturn_t (*)(char* version, unsigned int length);
	SYSTEM_GET_DRIVER_VERSION systemGetDriverVersion = load<SYSTEM_GET_DRIVER_VERSION>({"nvmlSystemGetDriverVersion"});

	/** DEVICE **/
	using DEVICE_GET_COUNT = nvmlReturn_t (*)(unsigned int* deviceCount);
	DEVICE_GET_COUNT deviceGetCount = load<DEVICE_GET_COUNT>({"nvmlDeviceGetCount_v2"});
	using DEVICE_GET_HANDLE_BY_INDEX = nvmlReturn_t (*)(unsigned int index, nvmlDevice_t* device);
	DEVICE_GET_HANDLE_BY_INDEX deviceGetHandleByIndex =
			load<DEVICE_GET_HANDLE_BY_INDEX>({"nvmlDeviceGetHandleByIndex_v2"});
	using DEVICE_GET_ARCHITECTURE = nvmlReturn_t (*)(nvmlDevice_t device, nvmlDeviceArchitecture_t* arch);
	DEVICE_GET_ARCHITECTURE deviceGetArchitecture = load<DEVICE_GET_ARCHITECTURE>({"nvmlDeviceGetArchitecture"});
	using DEVICE_GET_NAME = nvmlReturn_t (*)(nvmlDevice_t device, char* name, unsigned int length);
	DEVICE_GET_NAME deviceGetName = load<DEVICE_GET_NAME>({"nvmlDeviceGetName"});
	using DEVICE_GET_MEMORY_INFO = nvmlReturn_t (*)(nvmlDevice_t device, nvmlMemory_t* memory);
	DEVICE_GET_MEMORY_INFO deviceGetMemoryInfo = load<DEVICE_GET_MEMORY_INFO>({"nvmlDeviceGetMemoryInfo"});

#if defined(__linux__)
	NVMLLib() : msr::utils::SharedLib("libnvidia-ml.so.1") {}
#elif defined(__APPLE__)
/** \todo This should not be an error, the SharedLib instance should just not be "good" **/
#error "MacOS is not supported to fetch NVIDIA GPU information"
#elif defined(_WIN64)
/** \todo add support **/
#error "Support for windows needs to be added here"
#else
#error "Unsupported OS"
#endif
} nvml;

static const char* nvmlArchToStr(nvmlDeviceArchitecture_t arch) {
	switch (arch) {
	case NVML_DEVICE_ARCH_KEPLER:
		return "Kepler";
	case NVML_DEVICE_ARCH_MAXWELL:
		return "Maxwell";
	case NVML_DEVICE_ARCH_PASCAL:
		return "Pascal";
	case NVML_DEVICE_ARCH_VOLTA:
		return "Volta";
	case NVML_DEVICE_ARCH_TURING:
		return "Turing";
	case NVML_DEVICE_ARCH_AMPERE:
		return "Ampere";
	case NVML_DEVICE_ARCH_ADA:
		return "Ada";
	case NVML_DEVICE_ARCH_HOPPER:
		return "Hopper";
	}
	return "Unknown";
}

static bool initNVML() {
	if (!nvml.good())
		return false;
	switch (nvml.init()) {
	case NVML_SUCCESS:
		char buf[80];
		nvml.systemGetDriverVersion(buf, sizeof(buf) - 1);
		msr::log::info("gpustats", "NVML was loaded successfully with driver version {}", buf);
		return true;
	case NVML_ERROR_DRIVER_NOT_LOADED:
		msr::log::warn("gpustats", "No NVIDIA driver is running");
		break;
	case NVML_ERROR_NO_PERMISSION:
		msr::log::error("gpustats", "I don't have permission to talk to the driver");
		break;
	}
	return false;
}

GPUStats::GPUStats() : nvml({.supported = initNVML(), .devices = {}}) {
	if (!nvml.supported)
		return;
	unsigned int count;
	switch (::nvml.deviceGetCount(&count)) {
	case NVML_SUCCESS:
		msr::log::info("gpustats", "Found {} device(s):", count);
		for (unsigned i = 0u; i < count; ++i) {
			nvmlDevice_t device;
			switch (nvmlReturn_t ret; ret = ::nvml.deviceGetHandleByIndex(i, &device)) {
			case NVML_SUCCESS:
				nvmlDeviceArchitecture_t arch;
				::nvml.deviceGetArchitecture(device, &arch);
				char name[96];
				::nvml.deviceGetName(device, name, sizeof(name) - 1);
				msr::log::info("gpustats", "\t[{}] {} ({} Architecture)", i, name, nvmlArchToStr(arch));
				nvml.devices.emplace_back(device);
				break;
			default:
				msr::log::error("gpustats", "\t[{}] fetching handle failed with error {}", i, ::nvml.errorString(ret));
				break;
			}
		}
	}
}

void GPUStats::start() {
	if (!nvml.supported)
		return;
}
void GPUStats::stop() {
	if (!nvml.supported)
		return;
	/** \todo implement **/
}

#include <iostream>

void GPUStats::step() {
	if (!nvml.supported)
		return;
	nvmlMemory_t memory;
	/** \todo support multi-gpu **/
	for (auto device : nvml.devices) {
		if (nvmlReturn_t ret; (ret = ::nvml.deviceGetMemoryInfo(device, &memory)) == NVML_SUCCESS) {
			std::cout << "\r used/total (MB):\t" << memory.used / 1000 / 1000 << " / " << memory.total / 1000 / 1000
					  << std::flush;
			nvml.vramUsageTotal.addValue(memory.used / 1000 / 1000);
		} else {
			msr::log::critical("gpustats", "Could not fetch memory information: {}", ::nvml.errorString(ret));
			abort(); /** \todo how to handle? **/
		}
	}
}

Stats GPUStats::getStats() {
	/** \todo: filter by requested metrics */
	if (nvml.supported) {
		std::string vramTotal;
		for (auto& device : nvml.devices) {
			nvmlMemory_t memory;
			if (nvmlReturn_t ret; (ret = ::nvml.deviceGetMemoryInfo(device, &memory)) == NVML_SUCCESS) {
				vramTotal += std::to_string(memory.total / 1000 / 1000) + ",";
			} else {
				vramTotal += "None,";
			}
		}

		return {{MSR_GPU_SUPPORTED, "1"s},
				{MSR_GPU_MODEL_NAME, "TODO"s},
				{MSR_GPU_NUM_CORES, "TODO"s},
				{MSR_GPU_USED_PROCESS_PERCENT, "TODO"s},
				{MSR_GPU_USED_SYSTEM_PERCENT, "TODO"s},
				{MSR_GPU_VRAM_USED_PROCESS_MB, "TODO"s},
				{MSR_GPU_VRAM_USED_SYSTEM_MB, std::to_string(nvml.vramUsageTotal.maxValue())},
				{MSR_GPU_VRAM_AVAILABLE_SYSTEM_MB, vramTotal}};
	} else {
		return {{MSR_GPU_SUPPORTED, "0"s}};
	}
}