#include <pro.h>
#include <ida.hpp>
#include <idp.hpp>
#include <kernwin.hpp>
#include <loader.hpp>
#include <string>
#include "plugin.h"
#include "iscript.h"

std::vector<IScript *> g_Scripts;

static plugmod_t *idaapi init()
{
	addon_info_t addon_info;
	addon_info.cb = sizeof(addon_info_t);
	addon_info.id = sourcemod::kPluginID;
	addon_info.name = sourcemod::kPluginName;
	addon_info.producer = sourcemod::kPluginAuthor;
	addon_info.version = sourcemod::kPluginVer;
	addon_info.freeform = sourcemod::kPluginFreeForm;
	register_addon(&addon_info);

	create_menu(sourcemod::kPluginName, sourcemod::kPluginMenuName);
	for (IScript *p : g_Scripts)
	{
		if (p == nullptr)
			continue;

		if (!p->OnLoad())
			continue;

		const char *pPath = p->GetPath();
		// Extreme laziness
		std::string path = pPath == nullptr || pPath == "" ? std::string(sourcemod::kPluginMenuName) + std::string("/") : std::string(pPath);

		ICallableScript *pCallable = dynamic_cast<ICallableScript *>(p);
		if (pCallable != nullptr)
		{
			// msg("attaching %s %s %d\n", path.c_str(), p->GetLabel(), p->GetActionFlags());
			attach_action_to_menu(path.c_str(), p->GetLabel(), p->GetActionFlags());
		}
		p->OnLoadPost();
	}

	msg("SourceMod Utils loaded\n");
	return new sourcemod::plugin_ctx_t;
}

sourcemod::plugin_ctx_t::~plugin_ctx_t()
{
	for (IScript *p : g_Scripts)
	{
		if (p)
			p->OnUnload();
	}
	delete_menu(sourcemod::kPluginName);
	g_Scripts.clear();
}

plugin_t PLUGIN
{
	IDP_INTERFACE_VERSION,
	sourcemod::kPluginFlags,
	init,
	nullptr,
	nullptr,
	sourcemod::kPluginComment,
	nullptr,
	sourcemod::kPluginMenuName,
	nullptr
};

plugin_t *sourcemod::GetPlugin(void)
{
	return &PLUGIN;
}