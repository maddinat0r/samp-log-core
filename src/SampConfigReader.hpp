#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "CSingleton.hpp"


class SampConfigReader : public CSingleton<SampConfigReader>
{
	friend class CSingleton<SampConfigReader>;
private:
	SampConfigReader();
	~SampConfigReader() = default;

public:
	bool GetVar(const char *varname, std::string &dest) const;
	bool GetVarList(const char *varname, std::vector<std::string> &dest) const;
	bool GetGamemodeList(std::vector<std::string> &dest) const;

private:
	std::unordered_map<std::string, std::string> _settings;

};
