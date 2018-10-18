#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "Singleton.hpp"


class SampConfigReader : public Singleton<SampConfigReader>
{
	friend class Singleton<SampConfigReader>;
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
