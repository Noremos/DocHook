#pragma once


#include "../Bind/Json.h"


//import BackBind;

class JsonObjectIOState;

class JsonArrayIOState;
class JsonArrayIOStateReader;
class JsonArrayIOStateWriter;

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
		val = json[name];
	}

	void scInt(const BackString& name, int& val)
	{
		val = json[name];
	}
	void scFloat(const BackString& name, float& val)
	{
		val = json[name];
	}
	void scDouble(const BackString& name, double& val)
	{
		val = json[name];
	}
	void scStr(const BackString& name, BackString& val)
	{
		val = json[name];
	}
	void scPath(const BackString& name, BackPathStr& val)
	{
		val = (BackString)json[name];
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
		BackJson& refjarr = json[name] = BackJson::array();
		for (size_t i = 0; i < size; i++)
			refjarr.push_back(0);

		curArray = std::make_unique<TAW>(refjarr);
		return curArray.get();
	}

	JsonObjectIOState* objectBegin(const BackString& name)
	{

		BackJson& refjobj = json[name] = BackJson::object();
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
		val = static_cast<buchar>((uint)json[ind]);
	}

	void scBool(int ind, bool& val)
	{
		val = json[ind];
	}

	void scInt(int ind, int& val)
	{
		val = json[ind];
	}
	void scFloat(int ind, float& val)
	{
		val = json[ind];
	}
	void scDouble(int ind, double& val)
	{
		val = json[ind];
	}
	void scStr(int ind, BackString& val)
	{
		val = json[ind];
	}
	void scPath(int ind, BackPathStr& val)
	{
		val = (std::string)json[ind];
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

		BackJson& refjarr = json[ind] = BackJson::array();
		for (size_t i = 0; i < size; i++)
		{
			refjarr.push_back(0);
		}
		curArray = std::make_unique<JsonArrayIOStateWriter>(refjarr);
		return dynamic_cast<JsonArrayIOState*>(curArray.get());
	}

	JsonObjectIOState* objectBegin(int ind)
	{
		BackJson& refobj = json[ind] = BackJson::object();
		curObj = std::make_unique<TJsonObjectIOStateWriter<JsonArrayIOStateWriter>>(refobj);
		return dynamic_cast<JsonObjectIOState*>(curObj.get());
	}

	TJsonObjectIOStateWriter<JsonArrayIOStateWriter>* append()
	{
		curObj = std::make_unique<TJsonObjectIOStateWriter<JsonArrayIOStateWriter>>(json.emplace_back());
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


using JsonObjectIOStateReader = TJsonObjectIOStateReader<JsonArrayIOStateReader>;
using JsonObjectIOStateWriter = TJsonObjectIOStateWriter<JsonArrayIOStateWriter>;