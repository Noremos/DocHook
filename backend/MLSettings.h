#ifdef USE_MODULE
module;
#else
#pragma once
#endif

#include <cstdint>
#include <vector>
#include "../Bind/Json.h"
#include "Usings.h"

#ifdef USE_MODULE
export module MLSettings;
#undef MEXPORT
#define MEXPORT export
#else
#undef MEXPORT
#define MEXPORT
#endif

// import JsonCore;
//import BackBind;

MEXPORT struct OptionEnum
{
	OptionEnum(std::initializer_list<BackString> val)
	{
		names = val;
		for (size_t i = 0; i < names.size(); i++)
		{
			values.push_back(i);
		}
	}

	template<class T>
	void add(const BackString& name, T value)
	{
		names.push_back(name);
		values.push_back((int)value);
	}

	BackStringView getSelectedName() const
	{
		return names[selected];
	}

	int getSelectedValue() const
	{
		return values[selected];
	}
	void clear()
	{
		values.clear();
		names.clear();
	}


	std::vector<BackString> names;
	std::vector<int> values;
	int selected = 0;
	int64_t filter = INT64_MAX;
};

MEXPORT union OptionVariant
{
	bool b;
	int i;
	double d;
	BackString* s;
	BackPathStr* p;
	OptionEnum* e;
};


MEXPORT struct OptionValue
{
	OptionVariant data;
	BackString name;

	enum OptionVariantType
	{
		sv_bool,
		sv_int,
		sv_double,
		sv_str,
		sv_path,
		sv_enum
	} type;

public:
	void operator =(OptionValue&& other)
	{
		name = std::exchange(other.name, "");
		type = std::exchange(other.type, sv_bool);
		data = std::move(other.data);
		other.data.b = false;
	}

	OptionValue(OptionValue&& other)
	{
		*this = std::move(other);
	}

	OptionValue& operator =(const OptionValue& other)
	{
		name = other.name;
		type = other.type;
		switch (type)
		{
		case OptionValue::sv_str:
			data.s = new BackString(*other.data.s);
			break;
		case OptionValue::sv_path:
			data.p = new BackPathStr(*other.data.p);
			break;
		case OptionValue::sv_enum:
			data.e = new OptionEnum({});
			data.e->values = other.data.e->values;
			data.e->names = other.data.e->names;
			data.e->selected = other.data.e->selected;
			break;
		default:
			data = other.data;
			break;
		}

		return *this;
	}

	OptionValue(const OptionValue& other)
	{
		*this = other;
	}


	OptionValue(const BackString& name, bool val)
	{
		this->name = name;
		data.b = val;
		type = sv_bool;
	}

	OptionValue(const BackString& name, int val)
	{
		this->name = name;
		data.i = val;
		type = sv_int;
	}

	OptionValue(const BackString& name, double val)
	{
		this->name = name;
		data.d = val;
		type = sv_double;
	}

	OptionValue(const BackString& name, BackString val)
	{
		this->name = name;
		data.s = new BackString(val);
		type = sv_str;
	}
	//OptionValue(const BackString& name, BackPathStr val)
	//{
	//	this->name = name;
	//	data.p = &val;
	//	type = sv_path;
	//}

	OptionValue(const BackString& name, std::initializer_list<BackString> val)
	{
		this->name = name;
		data.e = new OptionEnum(val);
		type = sv_enum;
	}
	~OptionValue()
	{
		switch (type)
		{
		case OptionValue::sv_str:
			delete data.s;
			break;
		case OptionValue::sv_path:
			delete data.p;
			break;
		case OptionValue::sv_enum:
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
			json[name] = data.b;
			break;
		case sv_int:
			json[name] = data.i;
			break;
		case sv_double:
			json[name] = data.d;
			break;
		case sv_str:
			json[name] = *data.s;
			break;
		case sv_path:
			json[name] = data.p->string();
		case sv_enum:
		{
			JsonArray arr;
			arr.push_back(data.e->selected);
			for (size_t i = 0; i < data.e->values.size(); i++)
			{
				arr.push_back(data.e->names[i]);
				arr.push_back(data.e->values[i]);
			}
			json[name] = arr;
			break;
		}
		} //! switch
	}

	void readData(const BackJson& json)
	{
		switch (type)
		{
		case sv_bool:
			data.b = json[name];
			break;
		case sv_int:
			data.i = json[name];
			break;
		case sv_double:
			data.d = json[name];
			break;
		case sv_str:
			data.s = new BackString(json[name]);
			break;
		case sv_path:
			//*data.p = json[name].asString();
			throw;
			break;
		case sv_enum:
		{
			JsonArray arr = json[name].array();
			data.e = new OptionEnum({});
			data.e->selected = arr[0];
			for (int i = 1; i < arr.size(); i+=2)
			{
				data.e->names.push_back(arr[i]);
				data.e->values.push_back(arr[i+ 1]);
			}
			break;
		}
		}
	}
};

MEXPORT class MLSettings
{
public:
	std::vector<OptionValue> values;

	const bool* getBool(BackStringView name) const
	{
		for (auto& value : values)
		{
			if (StrEquals(value.name, name))
			{
				return &value.data.b;
			}
		}

		return nullptr;
	}

	void setBool(BackStringView name, bool val)
	{
		for (auto& value : values)
		{
			if (StrEquals(value.name, name))
			{
				value.data.b = val;
			}
		}
	}

	const BackString* getString(BackStringView name) const
	{
		for (auto& value : values)
		{
			if (StrEquals(value.name, name))
			{
				return value.data.s;
			}
		}

		return nullptr;
	}

	const int* getInt(BackStringView name) const
	{
		for (auto& value : values)
		{
			if (StrEquals(value.name, name))
			{
				return &value.data.i;
			}
		}

		return nullptr;
	}

	int* getInt(BackStringView name)
	{
		for (auto& value : values)
		{
			if (StrEquals(value.name, name))
			{
				return &value.data.i;
			}
		}

		return nullptr;
	}

	const double* getDouble(BackStringView name) const
	{
		for (auto& value : values)
		{
			if (StrEquals(value.name, name))
			{
				return &value.data.d;
			}
		}

		return nullptr;
	}
	BackStringView getEnum(BackStringView name) const
	{
		for (auto& value : values)
		{
			if (StrEquals(value.name, name))
			{
				return value.data.e->getSelectedName();
			}
		}

		throw;
	}

	template<class T>
	T getEnumValue(BackStringView name) const
	{
		for (auto& value : values)
		{
			if (value.name == name)
			{
				return static_cast<T>(value.data.e->getSelectedValue());
			}
		}

		throw;
	}

	MLSettings(std::initializer_list<OptionValue> l = {}) : values(l)
	{ }

	void write(BackJson& json) const
	{
		for (auto& set : values)
		{
			set.writeData(json);
		}
	}
	void read(const BackJson& json)
	{
		for (auto& set : values)
		{
			set.readData(json);
		}
	}
};
