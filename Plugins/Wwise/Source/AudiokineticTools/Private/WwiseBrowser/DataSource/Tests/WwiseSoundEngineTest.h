/*******************************************************************************
The content of this file includes portions of the proprietary AUDIOKINETIC Wwise
Technology released in source code form as part of the game integration package.
The content of this file may not be used without valid licenses to the
AUDIOKINETIC Wwise Technology.
Note that the use of the game engine is subject to the Unreal(R) Engine End User
License Agreement at https://www.unrealengine.com/en-US/eula/unreal

License Usage

Licensees holding valid licenses to the AUDIOKINETIC Wwise Technology may use
this file in accordance with the end user license agreement provided with the
software or, alternatively, in accordance with the terms contained
in a written agreement between you and Audiokinetic Inc.
Copyright (c) 2026 Audiokinetic Inc.
*******************************************************************************/

#pragma once

#include "Wwise/WwiseUnitTests.h"

#if WWISE_UNIT_TESTS

#include "Wwise/API_Null/WwiseSoundEngineAPI_Null.h"
#include "Wwise/WwiseSoundEngineModule.h"
#include "Misc/Crc.h"

// Temporary port of the FWwiseSoundEngineAPI_Test for 2025.1 as we need a proper definition of GetIDFromString for a few Merge tests.
class FWwiseSoundEngineAPI_Test : public FWwiseSoundEngineAPI_Null
{
public:
	virtual AkUInt32 GetIDFromString(const char* in_pszString) override
	{
		// Match Wwise's FNV-1 hash with lowercase conversion
		// See AK::FNVHash32::ComputeLowerCase in AkFNVHash.h
		if (!in_pszString)
		{
			return AK_INVALID_UNIQUE_ID;
		}
	
		constexpr AkUInt32 FNV_OFFSET_BASIS = 2166136261u;
		constexpr AkUInt32 FNV_PRIME = 16777619u;
	
		AkUInt32 Hash = FNV_OFFSET_BASIS;
		for (const char* p = in_pszString; *p; ++p)
		{
			Hash *= FNV_PRIME;  // FNV-1: multiply first
			// Convert to lowercase (ASCII only) then XOR
			unsigned char c = static_cast<unsigned char>(*p);
			if (c >= 'A' && c <= 'Z')
			{
				c = c - 'A' + 'a';
			}
			Hash ^= c;
		}
		return Hash;
	}
};

// Swaps the process-wide IWwiseSoundEngineModule::SoundEngine pointer for the fake for the
// scope's lifetime, restoring the previous pointer on destruction.
class FScopedSoundEngineOverride_ForTesting
{
public:
	FScopedSoundEngineOverride_ForTesting()
		: Previous(IWwiseSoundEngineModule::SoundEngine)
	{
		IWwiseSoundEngineModule::SoundEngine = &TestSoundEngine;
	}

	~FScopedSoundEngineOverride_ForTesting()
	{
		IWwiseSoundEngineModule::SoundEngine = Previous;
	}

private:
	FWwiseSoundEngineAPI_Test TestSoundEngine;
	IWwiseSoundEngineAPI* Previous;
};

#endif // WWISE_UNIT_TESTS
