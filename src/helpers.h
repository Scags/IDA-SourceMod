#pragma once
#ifndef HELPERS_INCLUDED
#define HELPERS_INCLUDED

#include <pro.h>
#include <ida.hpp>
#include <bytes.hpp>
#include <string>
#include <format>

inline std::string print_wildcards(size_t size)
{
	std::string out;
	for (size_t i = 0; i < size; i++)
	{
		out += "? ";
	}
	return out;
}

inline bool is_good_sig(std::string &sig)
{
	static ea_t funcs_segend = get_segm_by_name(".text")->end_ea;
	int count = 0;
	ea_t addr = 0;
	compiled_binpat_vec_t vec;
	if (!parse_binpat_str(&vec, addr, sig.c_str(), 16))
	{
		return false;
	}

	addr = bin_search2(addr, funcs_segend, vec, BIN_SEARCH_FORWARD);
	while (count < 2 && addr != BADADDR)
	{
		addr += 1; // bin_search2 is goofy
		count += 1;
		addr = bin_search2(addr, funcs_segend, vec, BIN_SEARCH_FORWARD);
	}
	return count == 1;
}

enum
{
	SIGFAIL_NONE = 0,
	SIGFAIL_OOF = 1,
	SIGFAIL_INSN = 2,
	SIGFAIL_LENGTH = 3
};

inline int makesig(ea_t foundaddr, std::string &smsig, std::string *sigout = nullptr)
{
	func_t *pFunc = get_func(foundaddr);
	if (!pFunc)
	{
		return SIGFAIL_OOF;
	}

	ea_t endaddr = pFunc->end_ea;

	ea_t addr = foundaddr;
	bool found = false;
	std::string sig;

	while (addr != BADADDR)
	{
		bool done = false;
		insn_t instruction;
		int length = decode_insn(&instruction, addr);
		if (length == 0)
		{
			return SIGFAIL_INSN;
		}

		if (instruction.Op1.type == o_near || instruction.Op1.type == o_far)
		{
			if (get_wide_byte(addr) == 0x0F)
			{
				sig += ("0F " + std::format("{:02X} ", get_wide_byte(addr + 1)) + print_wildcards(get_dtype_size(instruction.Op1.dtype)));
			}
			else
			{
				sig += std::format("{:02X} ", get_wide_byte(addr)) + print_wildcards(get_dtype_size(instruction.Op1.dtype));
			}
			done = true;
		}

		if (!done)
		{
			asize_t size = get_item_size(addr);
			for (asize_t i = 0; i < size; i++)
			{
				ea_t loc = addr + i;
				fixup_data_t fixup;
				fixup.get(loc);
				if ((fixup.get_type() & 0x0F) == FIXUP_OFF32)
				{
					sig += print_wildcards(4);
					i += 3;
				}
				else
				{
					sig += std::format("{:02X} ", get_wide_byte(loc));
				}
			}
		}
		if (is_good_sig(sig))
		{
			found = true;
			break;
		}
		addr = next_head(addr, endaddr);
	}

	if (sigout != nullptr)
		*sigout = sig;

	if (!found)
	{
		return SIGFAIL_LENGTH;
	}

	size_t length = sig.length() - 1;
	smsig = "\\x";
	for (size_t i = 0; i < length; i++)
	{
		char c = sig.at(i);
		if (c == ' ')
			smsig += "\\x";
		else if (c == '?')
			smsig += "2A";
		else
			smsig += c;
	}

	return SIGFAIL_NONE;
}

#endif	// HELPERS_INCLUDED