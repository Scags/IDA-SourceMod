#pragma once
#ifndef ISCRIPT_INCLUDED
#define ISCRIPT_INCLUDED

#include <pro.h>
#include <ida.hpp>
#include <idp.hpp>
#include <loader.hpp>
#include <kernwin.hpp>

class IScript
{
public:
	virtual bool OnLoad(void) = 0;
	virtual void OnLoadPost(void) = 0;
	virtual void OnUnload(void) = 0;
	virtual const char *GetName(void) const = 0;
	virtual const char *GetLabel(void) const = 0;
	virtual const char *GetPath(void) const = 0;
	virtual int GetActionFlags(void) = 0;
};

class ICallableScript
{
public:
	virtual action_desc_t *GetAction(void) = 0;
	virtual int Activate(action_activation_ctx_t *ctx) = 0;
	virtual action_state_t Update(action_update_ctx_t *ctx) = 0;
};

#endif 	// ISCRIPT_INCLUDED