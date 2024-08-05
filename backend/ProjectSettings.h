#ifdef USE_MODULE
#undef MEXPORT
#define MEXPORT export
module;
#else
#pragma once
#undef MEXPORT
#define MEXPORT
#endif

#include <memory>
#include <thread>

#ifdef USE_MODULE
export module ProjectSettings;
import MetadataCoreIO;
import RefSettings;
#else
#include "MetadataIO.h"
#include "Core/RefSettings.h"
#endif


MEXPORT struct ProjectSettings
{
	int metaCounter = 0;
	int threadsCount;
	bool runAsync = false;
	int layerCounter = 0;
	std::unique_ptr<MetadataProvider> metaprov;
	ProjectSettings()
	{
		threadsCount = std::thread::hardware_concurrency();
	}

	RefSettings& getIO()
	{
		return settings;
	}

	MetadataProvider& getMeta()
	{
		return *metaprov.get();
	}

private:
	RefSettings settings =
	{
		{"metacounter", metaCounter},
		{"threadsCount", threadsCount},
		{"runAsync", runAsync},
		{"layerCounter", layerCounter}
	};
};

MEXPORT static ProjectSettings& getSettings()
{
	static ProjectSettings globalSettings;
	return globalSettings;
}
