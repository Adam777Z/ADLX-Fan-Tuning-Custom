//
// Copyright (c) 2021 - 2023 Advanced Micro Devices, Inc. All rights reserved.
//
//-------------------------------------------------------------------------------------------------

/// \file main.cpp
/// \brief Set custom fan tuning with ADLX.

#include "SDK/ADLXHelper/Windows/Cpp/ADLXHelper.h"
#include "SDK/Include/IGPUManualFanTuning.h"
#include "SDK/Include/IGPUTuning.h"
#include <iostream>

// Use ADLX namespace
using namespace adlx;

// ADLXHelper instance
// No outstanding interfaces from ADLX must exist when ADLX is destroyed.
// Use global variables to ensure validity of the interface.
static ADLXHelper g_ADLXHelp;

// Wait for exit with error message
int WaitAndExit(const char* msg, const int retCode);

// Set Zero RPM setting
void SetZeroRPM(IADLXManualFanTuningPtr manualFanTuning, adlx_bool enabled);

// Set fan settings
void SetFan(IADLXManualFanTuningPtr manualFanTuning);

int main()
{
	ADLX_RESULT res = ADLX_FAIL;

	// Initialize ADLX
	res = g_ADLXHelp.Initialize();

	if (ADLX_SUCCEEDED(res))
	{
		IADLXGPUTuningServicesPtr gpuTuningService;
		res = g_ADLXHelp.GetSystemServices()->GetGPUTuningServices(&gpuTuningService);
		if (ADLX_FAILED(res))
		{
			// Destroy ADLX
			res = g_ADLXHelp.Terminate();
			std::cout << "Destroy ADLX res: " << res << std::endl;
			return WaitAndExit("\tGet GPU tuning services failed", 0);
		}
		IADLXGPUListPtr gpus;
		res = g_ADLXHelp.GetSystemServices()->GetGPUs(&gpus);
		if (ADLX_FAILED(res))
		{
			// Destroy ADLX
			res = g_ADLXHelp.Terminate();
			std::cout << "Destroy ADLX res: " << res << std::endl;
			return WaitAndExit("\tGet GPU list failed", 0);
		}
		IADLXGPUPtr oneGPU;
		res = gpus->At(0, &oneGPU);
		if (ADLX_FAILED(res) || oneGPU == nullptr)
		{
			// Destroy ADLX
			res = g_ADLXHelp.Terminate();
			std::cout << "Destroy ADLX res: " << res << std::endl;
			return WaitAndExit("\tGet GPU failed", 0);
		}
		adlx_bool supported = false;
		res = gpuTuningService->IsSupportedManualFanTuning(oneGPU, &supported);
		if (ADLX_FAILED(res) || supported == false)
		{
			// Destroy ADLX
			res = g_ADLXHelp.Terminate();
			std::cout << "Destroy ADLX res: " << res << std::endl;
			return WaitAndExit("\tThis GPU doesn't supported manual fan tuning", 0);
		}
		IADLXInterfacePtr manualFanTuningIfc;
		res = gpuTuningService->GetManualFanTuning(oneGPU, &manualFanTuningIfc);
		if (ADLX_FAILED(res) || manualFanTuningIfc == nullptr)
		{
			// Destroy ADLX
			res = g_ADLXHelp.Terminate();
			std::cout << "Destroy ADLX res: " << res << std::endl;
			return WaitAndExit("\tGet manual fan tuning interface failed", 0);
		}

		IADLXManualFanTuningPtr manualFanTuning(manualFanTuningIfc);
		if (manualFanTuning == nullptr)
		{
			// Destroy ADLX
			res = g_ADLXHelp.Terminate();
			std::cout << "Destroy ADLX res: " << res << std::endl;
			return WaitAndExit("\tGet manual fan tuning failed", 0);
		}

		// Set Custom Fan Tuning Settings
		SetZeroRPM(manualFanTuning, false);
		SetFan(manualFanTuning);
	}
	else
	{
		return WaitAndExit("\tg_ADLXHelp initialize failed", 0);
	}

	// Destroy ADLX
	res = g_ADLXHelp.Terminate();
	//std::cout << "Destroy ADLX res: " << res << std::endl;

	// Pause to see the print out
	//system("pause");

	return 0;
}

// Wait for exit with error message
int WaitAndExit(const char* msg, const int retCode)
{
	// Printout the message and pause to see it before returning the desired code
	if (nullptr != msg)
	{
		std::cout << msg << std::endl;
	}

	system("pause");
	return retCode;
}

// Set Zero RPM setting
void SetZeroRPM(IADLXManualFanTuningPtr manualFanTuning, adlx_bool enabled = false)
{
	adlx_bool supported = false;
	ADLX_RESULT res = manualFanTuning->IsSupportedZeroRPM(&supported);

	if (ADLX_FAILED(res) || !supported)
	{
		return;
	}

	adlx_bool ZeroRPMenabled = false;
	res = manualFanTuning->GetZeroRPMState(&ZeroRPMenabled);

	// Only set if current is different
	if (ZeroRPMenabled != enabled)
	{
		res = manualFanTuning->SetZeroRPMState(enabled);
	}
}

// Set fan settings
void SetFan(IADLXManualFanTuningPtr manualFanTuning)
{
	int FanSpeed[5] = { 25, 45, 50, 65, 90 };
	int Temperature[5] = { 45, 50, 65, 85, 95 };

	IADLXManualFanTuningStateListPtr states;
	IADLXManualFanTuningStatePtr oneState;
	ADLX_RESULT res = manualFanTuning->GetFanTuningStates(&states);
	if (ADLX_SUCCEEDED(res))
	{
		adlx_uint StatesSize = states->Size();

		if (StatesSize == 5)
		{
			for (adlx_uint crt = states->Begin(); crt != states->End(); ++crt)
			{
				res = states->At(crt, &oneState);
				adlx_int speed = 0, temperature = 0;

				oneState->GetFanSpeed(&speed);

				// Only set if current is different
				if (speed != FanSpeed[crt])
				{
					oneState->SetFanSpeed(FanSpeed[crt]);
					oneState->GetFanSpeed(&speed);
				}

				oneState->GetTemperature(&temperature);

				// Only set if current is different
				if (temperature != Temperature[crt])
				{
					oneState->SetTemperature(Temperature[crt]);
					oneState->GetTemperature(&temperature);
				}

				//std::cout << "\tThe current " << crt << " state: speed is " << speed << " temperature is " << temperature << std::endl;
			}
		}
	}

	// Set fan tuning states
	adlx_int errorIndex;
	res = manualFanTuning->IsValidFanTuningStates(states, &errorIndex);
	//std::cout << "\tIsValidGPUTuningStates, errorIndex is :" << errorIndex << std::endl;
	if (ADLX_SUCCEEDED(res))
	{
		manualFanTuning->SetFanTuningStates(states);
	}

	//res = manualFanTuning->GetFanTuningStates(&states);
	//if (ADLX_SUCCEEDED(res))
	//{
	//	std::cout << "\tAfter setting:" << std::endl;
	//	for (adlx_uint crt = states->Begin(); crt != states->End(); ++crt)
	//	{
	//		res = states->At(crt, &oneState);
	//		adlx_int speed = 0, temperature = 0;
	//		oneState->GetFanSpeed(&speed);
	//		oneState->GetTemperature(&temperature);
	//		std::cout << "\tThe current " << crt << " state: speed is " << speed << " temperature is " << temperature << std::endl;
	//	}
	//}
}