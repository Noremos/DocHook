// module;
#pragma once
#include <iostream>

#include "../side/json.hpp"
#include "Common.h"

//  module JsonCore;

//import BackBind;

using BackJson = nlohmann::json;
using JsonObject = nlohmann::json;
using JsonArray = nlohmann::json;


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

 	file << json;
 	file.close();
 }
