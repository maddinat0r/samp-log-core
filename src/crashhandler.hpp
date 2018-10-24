#pragma once


namespace crashhandler 
{
	void Install();

#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
	typedef DWORD Signal;
#else
	typedef int Signal;
#endif
}
