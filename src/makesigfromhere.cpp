#include <pro.h>
#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <funcs.hpp>
#include <ua.hpp>
#include <bytes.hpp>
#include <fixup.hpp>
#include <segment.hpp>
#include <search.hpp>

#include "scriptmgr.h"
#include "plugin.h"
#include "helpers.h"

#include <concepts>
#include <format>
#include <string>

class MakeSigFromHere : public CallableScript
{
public:
	MakeSigFromHere(const char *pName)
		: CallableScript(pName, pName, nullptr, 0) {}
	MakeSigFromHere(const char *pName, const char *pLabel, const char *pPath = nullptr, int actionflags = 0)
		: CallableScript(pName, pLabel, pPath, actionflags)	{}

	virtual bool OnLoad(void)
	{
		SetupAction("Ctrl+Shift+D");
		return true;
	}

	virtual int Activate(action_update_ctx_t *ctx)
	{
		ea_t addr = get_screen_ea();
		std::string smsig;
		std::string sig;
		int status = makesig(addr, smsig, &sig);
		switch (status)
		{
			case SIGFAIL_OOF:
				warning("Make sure you are in a function!");
				break;
			case SIGFAIL_INSN:
				msg("Something awful happened!\n");
				break;
			case SIGFAIL_LENGTH:
				msg("Ran out of bytes creating a unique signature.\n");
				msg("%s\n", sig.c_str());
				break;
			case SIGFAIL_NONE:
				qstring funcname;
				get_func_name(&funcname, addr);
				msg("Signature for %s at %X (offset %lu):\n%s\n%s\n", funcname.c_str(), addr, (addr - get_func(addr)->start_ea), sig.c_str(), smsig.c_str());
				break;
		}
		return 0;
	}
};

static MakeSigFromHere script("MakeSig from here");