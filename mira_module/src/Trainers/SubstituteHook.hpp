#pragma once
#include <cstdint>

// Driver includes
#include <mira/Driver/DriverCmds.hpp>
#include <mira/Driver/DriverStructs.hpp>

namespace Mira
{
    class SubstituteHook
    {
	public:
	    /**
	     * @brief Find the Jumpslot address for a given function
	     * 
	     * @param module Process Id of the target process to read
	     * @param p_ProcessAddress Address in the target process to read
	     * @return address of the function offset inside the process, NULL if not found (or error)
	     */
	    static void* FindJmpslotAddress(char* p_Module, char* p_Function, bool p_isNid);

    };
}