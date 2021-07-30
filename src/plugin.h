#pragma once
#ifndef PLUGIN_INCLUDED
#define PLUGIN_INCLUDED

#include <pro.h>
#include <ida.hpp>
#include <idp.hpp>
#include <kernwin.hpp>
#include <loader.hpp>

#include <vector>
#include "iscript.h"

namespace sourcemod
{
	// PLUGIN
	constexpr const int kPluginFlags = PLUGIN_FIX | PLUGIN_MULTI;
	constexpr const char kPluginComment[] = "View SourceMod scripts and utilities";
	constexpr const char kPluginMenuName[] = "SourceMod";

	const char kPluginName[] = "SourceMod Utils";
	const char kPluginVer[] = "1.1.0.4";
	const char kPluginAuthor[] = "Scag";
	const char kPluginID[] = "com.github.IDA-SourceMod";
	const char kPluginFreeForm[] = "(c) 2021, John Mascagni";

	struct plugin_ctx_t : public plugmod_t
	{
		~plugin_ctx_t();

		virtual bool idaapi run(size_t) override
		{
			return true;
		}
	};

	plugin_t *GetPlugin(void);
};

#endif	// PLUGIN_INCLUDED