#include "CSampConfigReader.hpp"

#include <fstream>
#include <algorithm>


CSampConfigReader::CSampConfigReader()
{
	std::ifstream config_file("server.cfg");
	while (config_file.good())
	{
		string line_buffer;
		std::getline(config_file, line_buffer);
		std::remove(line_buffer.begin(), line_buffer.end(), '\r');
		m_FileContent.push_back(std::move(line_buffer));
	}
}

bool CSampConfigReader::GetVar(string varname, string &dest)
{
	varname += ' ';
	for (auto &i : m_FileContent)
	{
		if (i.find(varname) == 0)
		{
			dest = i.substr(varname.length());
			return true;
		}
	}
	return false;
}

bool CSampConfigReader::GetVarList(string varname, vector<string> &dest)
{
	dest.clear();

	string data;
	if (GetVar(std::move(varname), data) == false)
		return false;

	size_t
		last_pos = 0,
		current_pos = 0;
	while (last_pos != string::npos)
	{
		if (last_pos != 0)
			++last_pos;

		current_pos = data.find(' ', last_pos);
		dest.push_back(data.substr(last_pos, current_pos - last_pos));
		last_pos = current_pos;
	}
	return dest.size() > 1;
}

bool CSampConfigReader::GetGamemodeList(vector<string> &dest)
{
	string
		varname("gamemode"),
		value;
	unsigned int counter = 0;

	while (GetVar(varname + std::to_string(counter), value))
	{
		dest.push_back(value.substr(0, value.find(' ')));
		++counter;
	}
	return counter != 0;
}
