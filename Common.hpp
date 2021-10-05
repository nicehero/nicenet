#ifndef ____NICE_COMMON___
#define ____NICE_COMMON___
#include <string>
namespace nicehero
{
	inline void splitString(const std::string& src, std::vector<std::string>& strvec, char splitWord)
	{
		std::string strtemp;
		std::string::size_type pos1, pos2;
		pos2 = src.find(splitWord);
		pos1 = 0;
		while (std::string::npos != pos2)
		{
			strvec.push_back(src.substr(pos1, pos2 - pos1));

			pos1 = pos2 + 1;
			pos2 = src.find(splitWord, pos1);
		}
		strvec.push_back(src.substr(pos1));
		return;
	}

	inline int StrCaseCmp(const std::string& a, const std::string& b) //same return 0
	{
		if (a.size() != b.size())
		{
			return 1;
		}
		for (size_t i = 0; i < a.size(); ++ i)
		{
			char x = a[i];
			char y = b[i];
			if (a[i] >= 'A' && a[i] <= 'Z')
			{
				x = (a[i] - ('A' - 'a'));
			}
			if (b[i] >= 'A' && b[i] <= 'Z')
			{
				y = (b[i] - ('A' - 'a'));
			}
			if (x != y)
			{
				return 1;
			}
		}
		return 0;
	}
}

#endif