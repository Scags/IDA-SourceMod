#define BYTES_SOURCE
#include <pro.h>
#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include <struct.hpp>

#include "scriptmgr.h"
#include "plugin.h"

#include <concepts>
#include <format>
#include <string>

// Hey you!
// Don't mind this file, but it's here in case you want it.
// It works, but IDA freezes for a solid 20 minutes when you run it.
// IDAPython must do some thread magic to not have the entire program freeze when you run a script.
// This is more or less of a "fire and forget" script, so if you wanna build and use it, go ahead.
// Just run it and then go touch some grass for a while. It's good for you.

class StructAlign : public CallableScript
{
public:
	StructAlign(const char *pName)
		: CallableScript(pName, pName, nullptr, 0) {}
	StructAlign(const char *pName, const char *pLabel, const char *pPath = nullptr, int actionflags = 0)
		: CallableScript(pName, pLabel, pPath, actionflags) {}

	virtual int Activate(action_update_ctx_t *ctx)
	{
		tid_t id = get_struc_id("CBaseEntity");
		if (id == BADADDR)
		{
			warning("You should only run this script after importing a netprop/datamap dump");
			return 0;
		}

		msg("Warning: This will take a while...\n");
		uval_t i = get_first_struc_idx();
		uval_t lastidx = get_last_struc_idx();

		while (i < lastidx)
		{
			struc_t *struc = get_struc(get_struc_by_idx(i));
			ea_t len = get_max_offset(struc);
			ea_t curr = 0;
			while (curr < len)
			{
				member_t *member = get_member(struc, curr);
				if (member == nullptr)
				{
					add_struc_member(struc, std::format("{x}").c_str(), curr, FF_BYTE, nullptr, 1);
					++curr;
				}
				else
				{
					curr += get_member_size(member);
				}
			}
		}
		msg("Done!\n");
		return 0;
	}
};

static StructAlign script("Align imported structures");