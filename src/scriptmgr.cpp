#include "scriptmgr.h"

Script::Script()
: m_MenuName(nullptr), m_MenuDescript(nullptr), m_Shortcut(nullptr)
{

}

void Script::Create(const char *pName, const char *pDesc, char *shortcut)
{
	m_MenuName = pName;
	m_MenuDescript = pDesc;
	m_Shortcut = shortcut;
}

void 