#ifdef USE_MODULE
#undef MEXPORT
#define MEXPORT export
module;
#else
#pragma once
#undef MEXPORT
#define MEXPORT
#endif


#include "../Bind/Json.h"

#ifdef USE_MODULE
export module JsonState;
#endif


//import BackBind;

MEXPORT class JsonObjectIOState;

MEXPORT class JsonArrayIOState;
MEXPORT class JsonArrayIOStateReader;
MEXPORT class JsonArrayIOStateWriter;

class JsonObjectIOState
{
public:
	virtual bool isReading() const = 0;
	virtual void scBool(const BackString& name, bool& val) = 0;
	virtual void scInt(const BackString& name, int& val) = 0;
	virtual void scFloat(const BackString& name, float& val) = 0;
	virtual void scDouble(const BackString& name, double& val) = 0;
	virtual void scStr(const BackString& name, BackString& val) = 0;
	virtual void scPath(const BackString& name, BackPathStr& val) = 0;
	virtual JsonArrayIOState* arrayBegin(const BackString& name, int& size) = 0;
	virtual JsonObjectIOState* objectBegin(const BackString& name) = 0;
	virtual void arrayEnd() = 0;
	virtual void objectEnd() = 0;
};

class JsonArrayIOState
{
public:
	virtual void scUchar(int ind, buchar& val) = 0;
	virtual void scInt(int ind, int& val) = 0;
	virtual void scFloat(int ind, float& val) = 0;
	virtual void scDouble(int ind, double& val) = 0;
	virtual void scStr(int ind, BackString& val) = 0;
	virtual void scPath(int ind, BackPathStr& val) = 0;
	virtual JsonArrayIOState* arrayBegin(int ind, int& size) = 0;
	virtual JsonObjectIOState* objectBegin(int ind) = 0;
	virtual void arrayEnd() = 0;
	virtual void objectEnd() = 0;
};

template<class TAR>
class TJsonObjectIOStateReader : public JsonObjectIOState
{
	const BackJson& json;
	std::unique_ptr<TAR> curArray;
	std::unique_ptr<TJsonObjectIOStateReader<TAR>> curObj;
public:
	TJsonObjectIOStateReader(const BackJson& json) : json(json)
	{}

	bool isReading() const
	{
		return true;
	}

	void scBool(const BackString& name, bool& val)
	{
		val = json[name].asBool();
	}

	void scInt(const BackString& name, int& val)
	{
		val = json[name].asInt();
	}
	void scFloat(const BackString& name, float& val)
	{
		val = json[name].asFloat();
	}
	void scDouble(const BackString& name, double& val)
	{
		val = json[name].asDouble();
	}
	void scStr(const BackString& name, BackString& val)
	{
		val = json[name].asString();
	}
	void scPath(const BackString& name, BackPathStr& val)
	{
		val = json[name].asString();
	}


	JsonArrayIOState* arrayBegin(const BackString& name, int& size)
	{
		const BackJson& jarr = json[name];
		size = jarr.size();
		curArray = std::make_unique<TAR>(jarr);
		return curArray.get();
	}

	JsonObjectIOState* objectBegin(const BackString& name)
	{
		const BackJson& jobj = json[name];
		curObj = std::make_unique<TJsonObjectIOStateReader<TAR>>(jobj);
		return curObj.get();
	}


	void arrayEnd()
	{
		curArray.reset(nullptr);
	}
	void objectEnd()
	{
		curObj.reset(nullptr);
	}
};


template<class TAW>
class TJsonObjectIOStateWriter : public JsonObjectIOState
{
	BackJson& json;
	std::unique_ptr<TAW> curArray;
	std::unique_ptr<TJsonObjectIOStateWriter<TAW>> curObj;

public:
	TJsonObjectIOStateWriter(BackJson& json) : json(json)
	{}

	bool isReading() const
	{
		return false;
	}

	void scInt(const BackString& name, int& val)
	{
		json[name] = val;
	}

	void scBool(const BackString& name, bool& val)
	{
		json[name] = val;
	}

	void scFloat(const BackString& name, float& val)
	{
		json[name] = val;
	}
	void scDouble(const BackString& name, double& val)
	{
		json[name] = val;
	}
	void scStr(const BackString& name, BackString& val)
	{
		json[name] = val;
	}
	void scPath(const BackString& name, BackPathStr& val)
	{
		json[name] = val.string();
	}


	JsonArrayIOState* arrayBegin(const BackString& name, int& size)
	{
		JsonArray jarr(Json::ValueType::arrayValue);
		json[name] = jarr;
		BackJson& refjarr = json[name];
		refjarr.resize(size);
		curArray = std::make_unique<TAW>(refjarr);
		return curArray.get();
	}

	JsonObjectIOState* objectBegin(const BackString& name)
	{
		JsonObject jobj(Json::ValueType::objectValue);
		json[name] = jobj;
		BackJson& refjobj = json[name];
		curObj = std::make_unique<TJsonObjectIOStateWriter<TAW>>(refjobj);
		return curObj.get();
	}


	void arrayEnd()
	{
		curArray.reset(nullptr);
	}
	void objectEnd()
	{
		curObj.reset(nullptr);
	}
};


class JsonArrayIOStateReader : public JsonArrayIOState
{
	const BackJson& json;
	std::unique_ptr<JsonArrayIOStateReader> curArray;
	std::unique_ptr<TJsonObjectIOStateReader<JsonArrayIOStateReader>> curObj;
public:
	JsonArrayIOStateReader(const BackJson& json) : json(json)
	{}

	void scUchar(int ind, buchar& val)
	{
		val = static_cast<buchar>(json[ind].asUInt());
	}

	void scBool(int ind, bool& val)
	{
		val = json[ind].isBool();
	}

	void scInt(int ind, int& val)
	{
		val = json[ind].asInt();
	}
	void scFloat(int ind, float& val)
	{
		val = json[ind].asFloat();
	}
	void scDouble(int ind, double& val)
	{
		val = json[ind].asDouble();
	}
	void scStr(int ind, BackString& val)
	{
		val = json[ind].asString();
	}
	void scPath(int ind, BackPathStr& val)
	{
		val = json[ind].asString();
	}

	JsonArrayIOState* arrayBegin(int ind, int& size)
	{
		const BackJson& jarr = json[ind];
		size = jarr.size();
		curArray = std::make_unique<JsonArrayIOStateReader>(jarr);
		return curArray.get();
	}

	JsonObjectIOState* objectBegin(int ind)
	{
		const BackJson& jobj = json[ind];
		curObj = std::make_unique<TJsonObjectIOStateReader<JsonArrayIOStateReader>>(jobj);
		return curObj.get();
	}

	void arrayEnd()
	{
		curArray.reset(nullptr);
	}
	void objectEnd()
	{
		curObj.reset(nullptr);
	}
};

class JsonArrayIOStateWriter : public JsonArrayIOState
{
	BackJson& json;
	std::unique_ptr<JsonArrayIOStateWriter> curArray;
	std::unique_ptr<TJsonObjectIOStateWriter<JsonArrayIOStateWriter>> curObj;

public:
	JsonArrayIOStateWriter(BackJson& json) : json(json)
	{}

	void scUchar(int ind, buchar& val)
	{
		json[ind] = val;
	}

	void scBool(int ind, bool& val)
	{
		json[ind] = val;
	}

	void scInt(int ind, int& val)
	{
		json[ind] = val;
	}
	void scFloat(int ind, float& val)
	{
		json[ind] = val;
	}
	void scDouble(int ind, double& val)
	{
		json[ind] = val;
	}
	void scStr(int ind, BackString& val)
	{
		json[ind] = val;
	}
	void scPath(int ind, BackPathStr& val)
	{
		json[ind] = val.string();
	}

	JsonArrayIOState* arrayBegin(int ind, int& size)
	{
		JsonArray jarr(Json::ValueType::arrayValue);
		json[ind] = jarr;
		BackJson& refjarr = json[ind];
		refjarr.resize(size);
		curArray = std::make_unique<JsonArrayIOStateWriter>(refjarr);
		return dynamic_cast<JsonArrayIOState*>(curArray.get());
	}

	JsonObjectIOState* objectBegin(int ind)
	{
		JsonArray jobj(Json::ValueType::objectValue);
		json[ind] = jobj;
		BackJson& refobj = json[ind];
		curObj = std::make_unique<TJsonObjectIOStateWriter<JsonArrayIOStateWriter>>(refobj);
		return dynamic_cast<JsonObjectIOState*>(curObj.get());
	}

	TJsonObjectIOStateWriter<JsonArrayIOStateWriter>* append()
	{
		curObj = std::make_unique<TJsonObjectIOStateWriter<JsonArrayIOStateWriter>>(json.append(BackJson()));
		return curObj.get();
	}

	void arrayEnd()
	{
		curArray.reset(nullptr);
	}
	void objectEnd()
	{
		curObj.reset(nullptr);
	}
};


MEXPORT using JsonObjectIOStateReader = TJsonObjectIOStateReader<JsonArrayIOStateReader>;
MEXPORT using JsonObjectIOStateWriter = TJsonObjectIOStateWriter<JsonArrayIOStateWriter>;