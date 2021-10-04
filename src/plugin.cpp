#include <pro.h>
#include <ida.hpp>
#include <idp.hpp>
#include <kernwin.hpp>
#include <loader.hpp>
#include <diskio.hpp>

#include "pugixml/pugixml.hpp"
#include "plugin.h"
#include "iscript.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace sourcemod
{
	std::vector<IScript *> scripts;
	std::unordered_map<std::string, std::string> shortcuts;
};

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

	std::string dir(idadir(CFG_SUBDIR) + std::string("\\sourcemod.xml"));
	auto xml = std::make_unique<pugi::xml_document>();
	pugi::xml_parse_result result = xml->load_file(dir.c_str());
	if (result.status != pugi::status_ok)
	{
		msg("SOURCEMOD: Could not parse config file '%s'\nError %d\n", dir.c_str(), result.status);
	}

	for (pugi::xml_node node : xml->child("shortcuts").children())
	{
		sourcemod::shortcuts[node.child_value("name")] = node.child_value("shortcut");
	}

	create_menu(sourcemod::kPluginName, sourcemod::kPluginMenuName);
	for (IScript *p : sourcemod::scripts)
	{
		if (p == nullptr)
			continue;

		if (!p->OnLoad())
			continue;

		const char *pPath = p->GetPath();
		// Extreme laziness
		std::string path(pPath == nullptr || pPath == "" ? std::string(sourcemod::kPluginMenuName) + "/" : pPath);

		ICallableScript *pCallable = dynamic_cast<ICallableScript *>(p);
		if (pCallable != nullptr)
		{
//			msg("attaching %s%s %s %d\n", path.c_str(), p->GetLabel(), p->GetName(), p->GetActionFlags());
			attach_action_to_menu(path.c_str(), p->GetLabel(), p->GetActionFlags());
		}
		p->OnLoadPost();
	}

	msg("SourceMod Utils loaded\n");
	return new sourcemod::plugin_ctx_t;
}

sourcemod::plugin_ctx_t::~plugin_ctx_t()
{
	for (IScript *p : scripts)
	{
		if (p)
			p->OnUnload();
	}
	delete_menu(sourcemod::kPluginName);
	scripts.clear();
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