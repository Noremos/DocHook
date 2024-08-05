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
#include <functional>
#include <istream>
#include <ostream>
#include "../Bind/Json.h"
#include "Usings.h"

#ifdef USE_MODULE
export module RefSettings;
import StateBinIO;
#else
#include "../../side/Barcode/PrjBarlib/modules/StateBinFile.h"
#endif

MEXPORT struct PathBlock
{
	BackPathStr* path;
	bool isFile;
	BackString filter;

	PathBlock(BackPathStr* path, bool isFile, BackString filter) : path(path), isFile(isFile), filter(filter)
	{ }

	PathBlock(const PathBlock& other) : path(other.path), isFile(other.isFile), filter(other.filter)
	{ }
};


MEXPORT struct EnumNameValue
{
	BackString name;
	int value;
};

MEXPORT struct EnumBlock
{
	int* selected = nullptr;
	int64_t filter = INT64_MAX;
	std::vector<EnumNameValue> enums;

	EnumBlock(int* selected, std::initializer_list<BackString> vals) : selected(selected)
	{
		int i = 0;
		for (const BackString& v : vals)
		{
			enums.push_back({v, i++});
		}
	}
	EnumBlock(const EnumBlock& other) : selected(other.selected), filter(other.filter), enums(other.enums)
	{ }

	EnumBlock(const JsonObject& obj)
	{
		readData(obj);
	}

	template<class T>
	EnumBlock(T* selected, std::initializer_list<std::pair<BackString, T>> vals) : selected(reinterpret_cast<int*>(selected))
	{
		for (const std::pair<BackString, T>& p : vals)
		{
			enums.push_back({p.first, (int)p.second});
		}
	}


	template<class T>
	void add(const BackString& name, T value)
	{
		enums.push_back({name, (int)value});
	}

	BackStringView getSelectedName() const
	{
		if (*selected < 0 || *selected >= (int)enums.size())
		{
			return "";
		}
		return enums[*selected].name;
	}
	int getSelectedValue() const
	{
		return enums[*selected].value;
	}
	void clear()
	{
		enums.clear();
	}

	JsonObject writeData()
	{
		JsonObject obj;
		obj["selected"] = *selected;
		obj["filter"] = filter;
		JsonArray arr;
		for (auto &e : enums)
		{
			arr.append(e.name);
			arr.append(e.value);
		}
		obj["enums"] = std::move(arr);

		return obj;
	}

	void readData(const JsonObject& obj)
	{
		*selected = obj["selected"].asInt();
		filter = obj["filter"].asInt64();

		JsonArray arr = obj["enums"];
		for (size_t i = 0; i < arr.size(); i++)
		{
			BackString name = arr.get(i, "").asString();
			int id = arr.get(i + 1, 0).asInt();
			enums.push_back({name, id});
		}
	}
};

MEXPORT union SettVariant
{
	bool* b;
	int* i;
	float* f;
	double* d;
	BackString* s;
	PathBlock* p;
	EnumBlock* e;
};

MEXPORT struct SettingValue
{
	BackString name;
	SettVariant data;

	enum SettVariantType
	{
		sv_bool,
		sv_int,
		sv_float,
		sv_double,
		sv_str,
		sv_path,
		sv_enum
	} type;

public:
	constexpr SettingValue(const BackString& name, bool& val)
	{
		this->name = name;
		data.b = &val;
		type = sv_bool;
	}

	constexpr SettingValue(const BackString& name, int& val)
	{
		this->name = name;
		data.i = &val;
		type = sv_int;
	}

	constexpr SettingValue(const BackString& name, float& val)
	{
		this->name = name;
		data.f = &val;
		type = sv_double;
	}

	constexpr SettingValue(const BackString& name, double& val)
	{
		this->name = name;
		data.d = &val;
		type = sv_double;
	}

	constexpr SettingValue(const BackString& name, BackString& val)
	{
		this->name = name;
		data.s = &val;
		type = sv_str;
	}

	constexpr SettingValue(const BackString& name, BackPathStr& val, bool fileMode, BackStringView filter = "")
	{
		this->name = name;
		data.p = new PathBlock(&val, fileMode, filter.data());
		type = sv_path;
	}

	constexpr SettingValue(const BackString& name, int* selected, std::initializer_list<BackString> vals)
	{
		this->name = name;
		data.e = new EnumBlock(selected, vals);
		type = sv_enum;
	}

	template<class T>
	constexpr SettingValue(const BackString& name, T& selected, std::initializer_list<std::pair<BackString, T>> vals)
	{
		this->name = name;
		data.e = new EnumBlock(&selected, vals);
		type = sv_enum;
	}

	constexpr SettingValue(const SettingValue& other)
	{
		operator=(other);
	}

	SettingValue(SettingValue&& other)
	{
		operator=(std::forward<SettingValue>(other));
	}

	void operator=(const SettingValue& other)
	{
		type = other.type;
		name = other.name;
		switch (type)
		{
		case sv_path:
			data.p = new PathBlock(*other.data.p);
			break;
		case sv_enum:
			data.e = new EnumBlock(*other.data.e);
			break;
		default:
			data = other.data;
			break;
		}
	}

	void operator=(SettingValue&& other)
	{
		type = other.type;
		name = std::move(other.name);
		data = std::move(other.data);
		other.type = sv_bool;
		other.name = "";
		other.data.b = nullptr;
	}

	~SettingValue()
	{
		switch (type)
		{
		case sv_path:
			delete data.p;
			break;
		case sv_enum:
			delete data.e;
			break;
		default:
			break;
		}
	}

	void writeData(BackJson& json) const
	{
		switch (type)
		{
		case sv_bool:
			json[name] = *data.b;
			break;
		case sv_int:
			json[name] = *data.i;
			break;
		case sv_float:
			json[name] = *data.f;
			break;
		case sv_double:
			json[name] = *data.d;
			break;
		case sv_str:
			json[name] = *data.s;
			break;
		case sv_path:
		{
			JsonObject pathobj;
			pathobj["path"] = data.p->path->string();
			pathobj["filter"] = data.p->filter;
			pathobj["isFile"] = data.p->isFile;
			json[name] = pathobj;
			break;
		}
		case sv_enum:
			json[name] =  data.e->writeData();
			break;
		default:
			break;
		}
	}

	void readData(const BackJson& json)
	{
		switch (type)
		{
		case sv_bool:
			*data.b = json[name].asBool();
			break;
		case sv_int:
			*data.i = json[name].asInt();
			break;
		case sv_float:
			*data.f = json[name].asFloat();
			break;
		case sv_double:
			*data.d = json[name].asDouble();
			break;
		case sv_str:
			*data.s = json[name].asString();
			break;
		case sv_path:
		{
			JsonObject pathobj = json[name];
			*data.p->path = pathobj["path"].asString();
			data.p->filter = pathobj["filter"].asString();
			data.p->isFile = pathobj["isFile"].asBool();
			break;
		}
		case sv_enum:
			data.e->readData(json[name]);
			break;
		default:
			break;
		}
	}
};

MEXPORT class RefSettings
{
public:
	std::vector<SettingValue> values;

	std::function<void(BackJson& json)> extraWrite;
	std::function<void(const BackJson& json)> extraRead;

	RefSettings(std::initializer_list<SettingValue> l = {}) : values(l)
	{ }

	// void add(std::initializer_list<SettingValue> l)
	// {
	// 	values.insert(values.end(), l.begin(), l.end());
	// }

	void add(const SettingValue& l)
	{
		values.push_back(l);
	}

	void write(BackJson& json) const
	{
		for (auto& set : values)
		{
			set.writeData(json);
		}

		if (extraWrite)
			extraWrite(json);
	}

	void read(const BackJson& json)
	{
		for (auto& set : values)
		{
			set.readData(json);
		}

		if (extraRead)
			extraRead(json);
	}
};
