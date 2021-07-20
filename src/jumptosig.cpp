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

class JumpToSig : public CallableScript
{
public:
	JumpToSig(const char *pName)
		: CallableScript(pName, pName, nullptr, 0) {}
	JumpToSig(const char *pName, const char *pLabel, const char *pPath = nullptr, int actionflags = 0)
		: CallableScript(pName, pLabel, pPath, actionflags) {}

	virtual bool OnLoad(void)
	{
		SetupAction("Ctrl+Shift+J");
		return true;
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

		ea_t addr = bin_search2(0, endea, vec, BIN_SEARCH_FORWARD);
		if (addr != BADADDR)
		{
			jumpto(addr, 0);
		}
		return 0;
	}
};

static JumpToSig script("Jump to signature");