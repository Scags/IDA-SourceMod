#pragma once
#ifndef SCRIPT_MGR_INCLUDED
#define SCRIPT_MGR_INCLUDED

#include <pro.h>
#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>
#include "iscript.h"
#include "plugin.h"

#include <vector>
#include <unordered_map>

namespace sourcemod
{
	extern std::vector<IScript *> scripts;
	extern std::unordered_map<std::string, std::string> shortcuts;
};

class Script : public IScript
{
public:
	// FIXME; having name and label as different values breaks adding to menu
	// Not super important but annoying nonetheless
	Script(const char *pName)
		: Script(pName, pName) {}
	Script(const char *pName, const char *pLabel, const char *pPath = nullptr, int actionflags = 0)
		: m_Name(pName), m_MenuName(pLabel), m_MenuPath(pPath), m_ActionFlags(actionflags)
	{
		sourcemod::scripts.push_back(this);
	}

	virtual bool OnLoad(void)
	{
		return true;
	}
	virtual void OnLoadPost(void) {}
	virtual void OnUnload(void) {}
	virtual const char *GetName(void) const
	{
		return m_Name;
	}
	virtual const char *GetLabel(void) const
	{
		return m_MenuName;
	}
	virtual const char *GetPath(void) const
	{
		return m_MenuPath;
	}
	virtual int GetActionFlags(void)
	{
		return m_ActionFlags;
	}

private:
	const char *m_Name;
	const char *m_MenuName;
	const char *m_MenuPath;
	int m_ActionFlags; // See SETMENU_ flags in kernwin.hpp
};

class CallableScript : public Script, public ICallableScript
{
public:
	CallableScript(const char *pName)
		: Script(pName, pName), m_Handler(this) {}
	CallableScript(const char *pName, const char *pLabel, const char *pPath = nullptr, int actionflags = 0)
		: Script(pName, pLabel, pPath, actionflags), m_Handler(this) {}

	virtual action_desc_t *GetAction(void)
	{
		return &m_Action;
	}
	virtual int Activate(action_activation_ctx_t *ctx)
	{
		return 0;
	}
	virtual action_state_t Update(action_update_ctx_t *ctx)
	{
		return AST_ENABLE_ALWAYS;
	}
	virtual bool OnLoad(void)
	{
		SetupAction();
		return true;
	}

	struct action_handler;
	inline action_handler *GetHandler(void)
	{
		return &m_Handler;
	}

	void SetupAction(const char *pShortcut = nullptr, const char *pTooltip = nullptr, int icon = 0, int flags = 0)
	{
		m_Action.cb = sizeof(m_Action);
		m_Action.name = GetName();
		m_Action.label = GetLabel();
		m_Action.handler = GetHandler();
		m_Action.owner = sourcemod::GetPlugin();
		m_Action.shortcut = sourcemod::shortcuts.contains(std::string(GetName())) ? sourcemod::shortcuts[std::string(GetName())].c_str() : pShortcut;
		m_Action.tooltip = pTooltip;
		m_Action.icon = icon;
		// ADF_ flags in kernwin.hpp
		m_Action.flags = flags;
		register_action(m_Action);
	}

	struct action_handler : public action_handler_t
	{
		action_handler(CallableScript *pOuter)
			: m_Out(pOuter) {}

		virtual int idaapi activate(action_activation_ctx_t *ctx)
		{
			return GetOuter()->Activate(ctx);
		}
		virtual action_state_t idaapi update(action_update_ctx_t *ctx)
		{
			return GetOuter()->Update(ctx);
		}

		inline CallableScript *GetOuter(void)
		{
			return m_Out;
		}
		CallableScript *m_Out;
	};

private:
	action_desc_t m_Action;
	action_handler m_Handler;
};

#endif // SCRIPT_MGR_INCLUDED