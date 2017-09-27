#include "SampConfigReader.hpp"

#include <fstream>
#include <algorithm>
#include <fmt/format.h>


SampConfigReader::SampConfigReader()
{
	std::ifstream config_file("server.cfg");
	while (config_file.good())
	{
		std::string line_buffer;
		std::getline(config_file, line_buffer);

		size_t cr_pos = line_buffer.find_first_of("\r\n");
		if (cr_pos != std::string::npos)
			line_buffer.erase(cr_pos);

		size_t split_pos = line_buffer.find(' ');
		if (split_pos != std::string::npos)
			_settings.emplace(line_buffer.substr(0, split_pos), line_buffer.substr(split_pos + 1));
	}
}

bool SampConfigReader::GetVar(const char *varname, std::string &dest) const
{
	if (_settings.find(varname) != _settings.end())
	{
		dest = _settings.at(varname);
		return true;
	}
	return false;
}

bool SampConfigReader::GetVarList(const char *varname, std::vector<std::string> &dest) const
{
	std::string data;
	if (GetVar(varname, data) == false)
		return false;

	dest.clear();

	size_t
		last_pos = 0,
		current_pos = 0;
	while (last_pos != std::string::npos)
	{
		if (last_pos != 0)
			++last_pos;

		current_pos = data.find(' ', last_pos);
		dest.push_back(data.substr(last_pos, current_pos - last_pos));
		last_pos = current_pos;
	}
	return true;
}

bool SampConfigReader::GetGamemodeList(std::vector<std::string> &dest) const
{
	std::string value;
	unsigned int counter = 0;

	dest.clear();

	while (GetVar(fmt::format("gamemode{}", counter).c_str(), value))
	{
		dest.push_back(value.substr(0, value.find(' ')));
		++counter;
	}
	return counter != 0;
}
