#pragma once

#include <string>
#include <vector>

#include "CSingleton.hpp"

using std::string;
using std::vector;


class CSampConfigReader : public CSingleton<CSampConfigReader>
{
	friend class CSingleton<CSampConfigReader>;
private:
	CSampConfigReader();
	virtual ~CSampConfigReader() = default;

public:
	bool GetVar(string varname, string &dest);
	bool GetVarList(string varname, vector<string> &dest);
	bool GetGamemodeList(vector<string> &dest);

private:
	vector<string> m_FileContent;
};
