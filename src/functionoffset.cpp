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

class GetOffsetToFunc : public CallableScript
{
public:
	GetOffsetToFunc(const char *pName)
		: CallableScript(pName, pName, nullptr, 0) {}
	GetOffsetToFunc(const char *pName, const char *pLabel, const char *pPath = nullptr, int actionflags = 0)
		: CallableScript(pName, pLabel, pPath, actionflags) {}

	virtual void OnLoadPost(void)
	{
		SetupAction("Ctrl+Shift+A");	// A for Awffset lol
	}

	virtual int Activate(action_update_ctx_t *ctx)
	{
		ea_t addr = get_screen_ea();
		func_t *pFunc = get_func(addr);
		if (pFunc == nullptr)
		{
			msg("Make sure you are in a function!\n");
			return 0;
		}

		msg("Offset from %X to %X:\n%d (0x%X)\n", pFunc->start_ea, addr, addr - pFunc->start_ea, addr - pFunc->start_ea);
		return 0;
	}
};

static GetOffsetToFunc script("Get offset from function");