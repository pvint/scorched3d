////////////////////////////////////////////////////////////////////////////////
//    Scorched3D (c) 2000-2004
//
//    This file is part of Scorched3D.
//
//    Scorched3D is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    Scorched3D is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with Scorched3D; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
////////////////////////////////////////////////////////////////////////////////

#if !defined(__INCLUDE_LUAScriptHook_INCLUDE__)
#define __INCLUDE_LUAScriptHook_INCLUDE__

#include <lua/LUAScriptFactory.h>
#include <map>
#include <vector>

class LUAScriptHook
{
public:
	class Param
	{
	public:
		enum Type
		{
			eString,
			eNumber,
			eBoolean
		};

		Param(fixed innumber) :
			number(innumber), type(eNumber) {};
		Param(const char *instr) :
			str(instr), type(eString) {};
		Param(const std::string &instr) :
			str(instr), type(eString) {};
		Param(bool b) :
			boolean(b), type(eBoolean) {};

		Type type;
		fixed number;
		bool boolean;
		std::string str;
	};

	LUAScriptHook(LUAScriptFactory *factory, 
		const std::string &hooksName,
		const std::string &directoryName);
	~LUAScriptHook();

	void addHookProvider(const std::string &hookName);

	void callHook(const std::string &hookName, const std::vector<Param> &params);
	void clearHooks();
	bool loadHooks();
	void listHooks();

	static std::vector<Param> formParam();
	static std::vector<Param> formParam(const Param &param1);
	static std::vector<Param> formParam(const Param &param1, const Param &param2);
	static std::vector<Param> formParam(const Param &param1, const Param &param2, const Param &param3);

protected:
	struct HookEntry
	{
		LUAScript *script;
		std::string entryPoint;
	};

	std::string directoryName_, hooksName_;
	LUAScriptFactory *factory_;
	std::map<std::string, std::vector<HookEntry> > hookNames_;
	bool loadHook(const std::string &directoryName, const std::string &fileName);
	void reloadHooks() { loadHooks(); }
};

#endif // __INCLUDE_LUAScriptHook_INCLUDE__
