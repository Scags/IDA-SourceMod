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

#include <regex>

class IsGoodSig : public CallableScript
{
public:
	IsGoodSig(const char *pName)
		: CallableScript(pName, pName, nullptr, 0) {}
	IsGoodSig(const char *pName, const char *pLabel, const char *pPath = nullptr, int actionflags = 0)
		: CallableScript(pName, pLabel, pPath, actionflags) {}

	virtual void OnLoadPost(void)
	{
		SetupAction("Ctrl+Shift+Z");
	}

	virtual int Activate(action_update_ctx_t *ctx)
	{
		qstring sig;
		// Dunno what to put for history here so w/e
		if (!ask_str(&sig, HIST_SRCH, "Insert signature"))
			return 0;

		if (sig.empty())
			return 0;

		std::string easysig = sig.c_str();
		easysig = std::regex_replace(easysig, std::regex("\\\\x"), " ");
		easysig = std::regex_replace(easysig, std::regex("2A"), "?");

		ea_t endea = get_segm_by_name(".text")->end_ea;
		compiled_binpat_vec_t vec;
		if (!parse_binpat_str(&vec, 0, &easysig.c_str()[1], 16))
		{
			return false;
		}

		int count = 0;

		ea_t addr = bin_search2(0, endea, vec, BIN_SEARCH_FORWARD);
		while (addr != BADADDR)
		{
			addr += 1;
			count += 1;
			addr = bin_search2(addr, endea, vec, BIN_SEARCH_FORWARD);
		}

		switch (count)
		{
			case 0:
				msg("INVALID: %s\nCould not find any matching signatures for input\n", sig.c_str());
				break;
			case 1:
				msg("VALID: %s\n", sig.c_str());
				break;
			default:
				msg("INVALID: %s\nFound %d instances of input signature\n", sig.c_str(), count);
				break;
		}
		return 0;
	}
};

static IsGoodSig script("Validate signature");