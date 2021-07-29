#define BYTES_SOURCE
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
#include <struct.hpp>
#include <typeinf.hpp>

#include "scriptmgr.h"
#include "plugin.h"
#include "helpers.h"

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <regex>

#include "pugixml/pugixml.hpp"

#define SPROP_UNSIGNED (1 << 0) // Unsigned integer data.
#define SPROP_COORD (1 << 1) // If this is set, the float/vector is treated like a world coordinate. \
							 // Note that the bit count is ignored in this case.
#define SPROP_NOSCALE (1 << 2) // For floating point, don't scale into range, just take value as is.
#define SPROP_ROUNDDOWN (1 << 3) // For floating point, limit high value to range minus one bit unit
#define SPROP_ROUNDUP (1 << 4) // For floating point, limit low value to range minus one bit unit
#define SPROP_NORMAL (1 << 5) // If this is set, the vector is treated like a normal (only valid for vectors)
#define SPROP_EXCLUDE (1 << 6) // This is an exclude prop (not excludED, but it points at another prop to be excluded).
#define SPROP_XYZE (1 << 7) // Use XYZ/Exponent encoding for vectors.
#define SPROP_INSIDEARRAY (1 << 8) // This tells us that the property is inside an array, so it shouldn't be put into the \
								   // flattened property list. Its array will point at it when it needs to.
#define SPROP_PROXY_ALWAYS_YES (1 << 9) // Set for datatable props using one of the default datatable proxies like \
										// SendProxy_DataTableToDataTable that always send the data to all clients.
#define SPROP_CHANGES_OFTEN (1 << 10) // this is an often changed field, moved to head of sendtable so it gets a small index
#define SPROP_IS_A_VECTOR_ELEM (1 << 11) // Set automatically if SPROP_VECTORELEM is used.
#define SPROP_COLLAPSIBLE (1 << 12) // Set automatically if it's a datatable with an offset of 0 that doesn't change the pointer \
									// (ie: for all automatically-chained base classes).                                         \
									// In this case, it can get rid of this SendPropDataTable altogether and spare the           \
									// trouble of walking the hierarchy more than necessary.
#define SPROP_COORD_MP (1 << 13)			  // Like SPROP_COORD, but special handling for multiplayer games
#define SPROP_COORD_MP_LOWPRECISION (1 << 14) // Like SPROP_COORD, but special handling for multiplayer games where the fractional component only gets a 3 bits instead of 5
#define SPROP_COORD_MP_INTEGRAL (1 << 15)	  // SPROP_COORD_MP, but coordinates are rounded to integral boundaries
#define SPROP_VARINT SPROP_NORMAL

#define SPROP_NUMFLAGBITS_NETWORKED 16

// This is server side only, it's used to mark properties whose SendProxy_* functions encode against gpGlobals->tickcount (the only ones that currently do this are
//  m_flAnimTime and m_flSimulationTime.  MODs shouldn't need to mess with this probably
#define SPROP_ENCODED_AGAINST_TICKCOUNT (1 << 16)

// See SPROP_NUMFLAGBITS_NETWORKED for the ones which are networked
#define SPROP_NUMFLAGBITS 17

typedef enum
{
	DPT_Unknown = -1,
	DPT_Int,
	DPT_Float,
	DPT_Vector,
	DPT_VectorXY,
	DPT_String,
	DPT_Array, // An array of the base types (can't be of datatables).
	DPT_DataTable,
#if 0 // We can't ship this since it changes the size of DTVariant to be 20 bytes instead of 16 and that breaks MODs!!!
	DPT_Quaternion,
#endif
	DPT_NUMSendPropTypes
} SendPropType;

static tinfo_t g_Vector;

class SendProp;
class SendTable;

class IServerClass
{
public:
	virtual ~IServerClass() {}
	virtual void Parse(pugi::xml_node node) = 0;
	virtual void MakeStruct(void) = 0;
	virtual std::string &GetName(void) = 0;
	virtual bool AlreadyParsed(void) = 0;
	virtual SendProp *GetProp(std::string &name) = 0;
	virtual void AddProp(std::string &name, std::shared_ptr<SendProp> prop) = 0;
	virtual SendTable *GetTable(std::string &name) = 0;
	virtual void AddTable(std::string &name, std::shared_ptr<SendTable> table) = 0;
};

class SendProp
{
public:
	SendProp(std::string name, IServerClass *cls)
		: m_Name(name), m_Class(cls), m_Offset(0), m_Bits(0), m_Flags(0), m_Type(DPT_Unknown), m_PredefinedSize(0) {}

	inline SendPropType DTTypeNameToType(const char *val)
	{
		if (!strcmp(val, "integer"))
		{
			return DPT_Int;
		}
		if (!strcmp(val, "float"))
		{
			return DPT_Float;
		}
		if (!strcmp(val, "vector"))
		{
			return DPT_Vector;
		}
		if (!strcmp(val, "string"))
		{
			return DPT_String;
		}
		if (!strcmp(val, "array"))
		{
			return DPT_Array;
		}
		if (!strcmp(val, "datatable"))
		{
			return DPT_DataTable;
		}
		if (!strcmp(val, "array"))
		{
			return DPT_Array;
		}

		return DPT_Unknown;
	}

	int StringToFlags(const char *val)
	{
		static std::unordered_map<std::string, int> flagmap{
			{"Unsigned", SPROP_UNSIGNED},
			{"Coord", SPROP_COORD},
			{"NoScale", SPROP_NOSCALE},
			{"RoundDown", SPROP_ROUNDDOWN},
			{"RoundUp", SPROP_ROUNDUP},
			{"Exclude", SPROP_EXCLUDE},
			{"XYZE", SPROP_XYZE},
			{"InsideArray", SPROP_INSIDEARRAY},
			{"AlwaysProxy", SPROP_PROXY_ALWAYS_YES},
			{"ChangesOften", SPROP_CHANGES_OFTEN},
			{"VectorElem", SPROP_IS_A_VECTOR_ELEM},
			{"Collapsible", SPROP_COLLAPSIBLE},
			{"CoordMP", SPROP_COORD_MP},
			{"CoordMPLowPrec", SPROP_COORD_MP_LOWPRECISION},
			{"CoordMpIntegral", SPROP_COORD_MP_INTEGRAL},
			{"VarInt", SPROP_NORMAL},
			{"Normal", SPROP_NORMAL}
		};
		std::string s(val);
		auto v = StringExplode(s, '|');

		int flags = 0;
		for (std::string flagstr : v)
		{
			flags |= flagmap[flagstr];
		}
		return flags;
	}

	flags_t GetSizeData(asize_t &bc)
	{
		switch (GetType())
		{
		case DPT_Int:
		{
			// Sometimes floats can be squished in here
			if (GetName().rfind("m_fl", 0) == 0)
			{
				bc = 4;
				return FF_FLOAT;
			}

			int highbyte = static_cast<int>(ceil(GetNumBits() / 8.0));
			switch (highbyte)
			{
			// If 0 bits we assume byte just so we don't knock anything out the way ahead of us
			case 0:
			case 1:
				bc = 1;
				return FF_BYTE;
			case 2:
				bc = 2;
				return FF_WORD;
			case 4:
				bc = 4;
				return FF_DWORD;
			default:
				bc = 1;
				return FF_BYTE;
			}
		}
		case DPT_Float:
			bc = 4;
			return FF_FLOAT;
		case DPT_Vector:
			bc = 12;
			return FF_FLOAT;
		case DPT_VectorXY:
			bc = 8;
			return FF_FLOAT;
		case DPT_String:
			bc = 4;
			return FF_DWORD;
		default:
			bc = 1;
			return FF_BYTE;
		}
	}

	inline std::string &GetName(void)
	{
		return m_Name;
	}
	inline ea_t GetOffset(void)
	{
		return m_Offset;
	}
	inline void SetOffset(ea_t offs)
	{
		m_Offset = offs;
	}
	inline int GetNumBits(void)
	{
		return m_Bits;
	}
	inline void SetNumBits(int bits)
	{
		m_Bits = bits;
	}
	inline int GetFlags(void)
	{
		return m_Flags;
	}
	inline void SetFlags(int flags)
	{
		m_Flags = flags;
	}
	inline SendPropType GetType(void)
	{
		return m_Type;
	}
	inline void SetType(SendPropType type)
	{
		m_Type = type;
	}
	inline asize_t GetPredefinedSize(void)
	{
		return m_PredefinedSize;
	}
	inline void SetPredefinedSize(asize_t size)
	{
		m_PredefinedSize = size;
	}

private:
	std::string m_Name;
	ea_t m_Offset;
	int m_Bits;
	int m_Flags;
	SendPropType m_Type;
	IServerClass *m_Class;
	ea_t m_PredefinedSize;
};

class SendTable
{
public:
	SendTable(std::string name, SendProp *ownerprop, IServerClass *cls)
		: m_Name(name), m_Owner(ownerprop), m_Class(cls) {}

	void Parse(pugi::xml_node node, ea_t curroffset = 0)
	{
// TODO; implement this
		// Very bad hack to get a decent size for embedded classes
//		std::string subprop(node.attribute("name"));
//		if (propname.rfind("DT_", 0) == 0)
//		{
//			std::string classname = std::string("C") + &propname.c_str()[3];
//			// This struct is probably gonna get parsed a trillion times
//			// Prevent this
//			if (get_struc_id(classname.c_str() == BADADDR)
//			{
//				// Can just grab struc later via classname
//				add_struc(BADADDR, classname.c_str());
//			}
//		}

		asize_t arraysize = 0;
		for (pugi::xml_node child : node.children())
		{
			std::string propname(child.attribute("name").value());
			if (propname == "")
			{
				continue;
			}
			if (propname == "baseclass")
			{
				Parse(child.child("sendtable"));
				continue;
			}

			// For the 000, 001 properties
			bool isarraymember = IsNumber(propname);
			int offset = atoi(child.child_value("offset"));
			if (isarraymember)
				arraysize = static_cast<asize_t>(atoi(propname.c_str()) * offset);

			auto prop = std::make_shared<SendProp>(propname, GetServerClass());

			std::string type(child.child_value("type"));
			prop->SetType(IsNumber(type) ? static_cast<SendPropType>(std::stoi(type)) : prop->DTTypeNameToType(type.c_str()));
			prop->SetOffset(static_cast<ea_t>(offset) + curroffset);
			prop->SetNumBits(atoi(child.child_value("bits")));
			prop->SetFlags(prop->StringToFlags(child.child_value("flags")));

//			msg("%s %s %d %lu %d %d\n", GetName(), propname.c_str(), prop->GetType(), prop->GetOffset(), prop->GetNumBits(), prop->GetFlags());
			AddProp(propname, prop);
			if (!isarraymember && prop->GetType() == DPT_DataTable)
			{
				auto table = std::make_shared<SendTable>(propname, GetProp(propname), GetServerClass());
				GetServerClass()->AddTable(propname, table);
				table->Parse(child.child("sendtable"), prop->GetOffset());
			}
		}
		if (OwnerProp() != nullptr && arraysize != 0)
			OwnerProp()->SetPredefinedSize(arraysize);
	}

	void AddToStruct(struc_t *struc)
	{
		if (OwnerProp() != nullptr && OwnerProp()->GetPredefinedSize() != BADADDR && Props().size() > 0 && OwnerProp()->GetOffset() > 0)
		{
			std::string &fieldname = GetName();
			ea_t offset = OwnerProp()->GetOffset();
			asize_t size;
			flags_t flags = OwnerProp()->GetSizeData(size);
			asize_t predefsize = OwnerProp()->GetPredefinedSize();
			add_struc_member(struc, fieldname.c_str(), offset, flags, nullptr, size * predefsize);
		}

		for (auto &val : Props())
		{
			auto prop = val.second;
			if (prop->GetOffset() == 0)
				continue;

			std::string &fieldname = prop->GetName();
			if (IsNumber(fieldname))
				continue;

//			msg("%s %lu\n", prop->GetName().c_str(), prop->GetOffset());
			ea_t offset = prop->GetOffset();
			asize_t size;
			flags_t flags = prop->GetSizeData(size);
			add_struc_member(struc, fieldname.c_str(), offset, flags, nullptr, size);
			if (prop->GetType() == DPT_Vector)
				set_member_tinfo(struc, get_member(struc, offset), 0, g_Vector, 0);
		}
	}

	inline std::string &GetName(void)
	{
		return m_Name;
	}

	inline IServerClass *GetServerClass(void)
	{
		return m_Class;
	}

	inline SendProp *GetProp(std::string &name)
	{
		return m_Props[name].get();
	}
	inline void AddProp(std::string &name, std::shared_ptr<SendProp> prop)
	{
		if (!m_Props.contains(name))
			m_Props[name] = prop;
	}
	inline std::map<std::string, std::shared_ptr<SendProp>> &Props(void)
	{
		return m_Props;
	}

	inline SendProp *OwnerProp(void)
	{
		return m_Owner;
	}

	inline bool IsSubTable(void)
	{
		return OwnerProp() && OwnerProp()->GetType() == DPT_DataTable;
	}

private:
	std::string m_Name;
	IServerClass *m_Class;
	std::map<std::string, std::shared_ptr<SendProp>> m_Props;
	SendProp *m_Owner;
};

class ImportManager;

class ServerClass : public IServerClass
{
public:
	ServerClass(std::string name)
		: m_Name(name), m_Skip(false)
	{
		m_StrucID = get_struc_id(name.c_str());
		if (m_StrucID == BADADDR)
		{
			m_StrucID = add_struc(BADADDR, name.c_str());
		}
		else
		{
			// Already exists. Someone ran the script again?
			m_Skip = true;
		}
	}

	virtual void Parse(pugi::xml_node node)
	{
		if (m_Skip)
			return;

		std::string tablename = std::string(node.attribute("name").value());
		auto table = std::make_shared<SendTable>(tablename, nullptr, this);
		AddTable(tablename, table);
		table->Parse(node);

		m_Skip = true;
	}

	virtual void MakeStruct(void)
	{
		struc_t *struc = GetStruct();
		msg("Adding %s\n", GetName());
		for (auto &val : Tables())
		{
			val.second->AddToStruct(struc);
		}
	}

	virtual std::string &GetName(void)
	{
		return m_Name;
	}

	virtual bool AlreadyParsed(void)
	{
		return m_Skip;
	}

	virtual SendProp *GetProp(std::string &name)
	{
		return m_Props[name].get();
	}

	virtual void AddProp(std::string &name, std::shared_ptr<SendProp> prop)
	{
		if (!m_Props.contains(name))
			m_Props[name] = prop;
	}

	virtual SendTable *GetTable(std::string &name)
	{
		return m_Tables[name].get();
	}

	virtual void AddTable(std::string &name, std::shared_ptr<SendTable> table)
	{
		if (!m_Tables.contains(name))
			m_Tables[name] = table;
	}

	inline tid_t GetStrucID(void)
	{
		return m_StrucID;
	}

	inline struc_t *GetStruct(void)
	{
		return get_struc(GetStrucID());
	}

	inline std::map<std::string, std::shared_ptr<SendProp>> &Props(void)
	{
		return m_Props;
	}

	inline std::map<std::string, std::shared_ptr<SendTable>> &Tables(void)
	{
		return m_Tables;
	}

private:
	std::string m_Name;
	std::map<std::string, std::shared_ptr<SendProp>> m_Props;
	std::map<std::string, std::shared_ptr<SendTable>> m_Tables;
	tid_t m_StrucID;
	bool m_Skip; // This struct already exists, this is most likely a reparse
};

class ImportManager
{
public:
	void Parse(pugi::xml_document *xml)
	{
		pugi::xml_node head = xml->child("netprops");
		for (pugi::xml_node node : head.children())
		{
			std::string classname(node.attribute("name").value());
			auto cls = std::make_shared<ServerClass>(classname);
			AddClass(classname, cls);
			if (!cls->AlreadyParsed())
				cls->Parse(node.first_child());
		}
	}

	void CreateStructs(void)
	{
		for (auto &val : Classes())
		{
			val.second->MakeStruct();
		}
	}

	void MakeBasicStructs(void)
	{
		tid_t strucid = get_struc_id("Vector");
		if (strucid == BADADDR)
		{
			strucid = add_struc(BADADDR, "Vector");
			struc_t *vector = get_struc(strucid);
			add_struc_member(vector, "x", BADADDR, FF_FLOAT, nullptr, sizeof(float));
			add_struc_member(vector, "y", BADADDR, FF_FLOAT, nullptr, sizeof(float));
			add_struc_member(vector, "z", BADADDR, FF_FLOAT, nullptr, sizeof(float));
		}

		qstring out;
		parse_decl(&g_Vector, &out, nullptr, "Vector;", 0);

		strucid = get_struc_id("QAngle");
		if (strucid == BADADDR)
		{
			strucid = add_struc(BADADDR, "QAngle");
			struc_t *vector = get_struc(strucid);
			add_struc_member(vector, "x", BADADDR, FF_FLOAT, nullptr, sizeof(float));
			add_struc_member(vector, "y", BADADDR, FF_FLOAT, nullptr, sizeof(float));
			add_struc_member(vector, "z", BADADDR, FF_FLOAT, nullptr, sizeof(float));
		}
	}

	inline std::shared_ptr<ServerClass> GetClass(std::string &name)
	{
		return m_ServerClasses[name];
	}

	inline void AddClass(std::string &name, std::shared_ptr<ServerClass> cls)
	{
		if (!m_ServerClasses.contains(name))
			m_ServerClasses[name] = cls;
	}

	inline std::map<std::string, std::shared_ptr<ServerClass>> &Classes(void)
	{
		return m_ServerClasses;
	}

private:
	std::map<std::string, std::shared_ptr<ServerClass>> m_ServerClasses;
};

class NetPropImporter : public CallableScript
{
public:
	NetPropImporter(const char *pName)
		: CallableScript(pName, pName, nullptr, 0) {}
	NetPropImporter(const char *pName, const char *pLabel, const char *pPath = nullptr, int actionflags = 0)
		: CallableScript(pName, pLabel, pPath, actionflags) {}

	virtual bool OnLoad(void)
	{
		SetupAction("Ctrl+Shift+N");
		return true;
	}

	virtual int Activate(action_update_ctx_t *ctx)
	{
		const char *file = ask_file(false, "*.xml", "Select a file to import");
		if (file == nullptr)
			return 0;

		auto xml = std::make_unique<pugi::xml_document>();
		pugi::xml_parse_result result = xml->load_file(file);
		if (result.status != pugi::status_ok)
		{
			warning("Could not parse file '%s'\nMake sure this netprop dump is from SM1.11+!\nError %d", file, result.status);
			return 0;
		}

		ImportManager mgr;
		mgr.MakeBasicStructs();
		mgr.Parse(xml.get());
		mgr.CreateStructs();

		return 0;
	}
};

static NetPropImporter script("Import netprop dump");