// module;
#pragma once
#include <iostream>

#include <json/json.h>
#include "Common.h"

//  module JsonCore;

//import BackBind;

using BackJson = Json::Value;
using JsonObject = BackJson;
using JsonArray = BackJson;


inline BackJson jsonFromFile(const BackPathStr& path)
 {
	BackJson temp;
 	BackFileReader file;
 	file.open(path);
 	if (!file.is_open())
 	{
 		std::cerr << "Couldn't open a file.";
 		return BackJson();
 	}
 	file >> temp;
 	file.close();

 	return temp;
 }

inline void jsonToFile(const BackJson& json, const BackPathStr& path)
 {
 	BackFileWriter file;
 	file.open(path, std::ios::trunc);
 	if (!file.is_open())
 	{
 		std::cerr << "Couldn't open a file.";
 		return;
 	}

    Json::StreamWriterBuilder wbuilder;
    // Configure the Builder, then ...
    std::string outputConfig = Json::writeString(wbuilder, json);
 	file << outputConfig;
 	file.close();
 }
