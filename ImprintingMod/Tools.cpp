#include <windows.h>
#include <fstream>
#include <chrono>
#include <ctime>
#include "Tools.h"

AShooterPlayerController* FindPlayerControllerFromSteamId(unsigned __int64 steamId)
{
	AShooterPlayerController* result = nullptr;

	auto playerControllers = Ark::GetWorld()->GetPlayerControllerListField();
	for (uint32_t i = 0; i < playerControllers.Num(); ++i)
	{
		auto playerController = playerControllers[i];

		APlayerState* playerState = playerController->GetPlayerStateField();
		__int64 currentSteamId = playerState->GetUniqueIdField()->UniqueNetId->GetUniqueNetIdField();

		if (currentSteamId == steamId)
		{
			AShooterPlayerController* aShooterPC = static_cast<AShooterPlayerController*>(playerController.Get());

			result = aShooterPC;
			break;
		}
	}

	return result;
}

wchar_t* ConvertToWideStr(const std::string& str)
{
	size_t newsize = str.size() + 1;

	wchar_t* wcstring = new wchar_t[newsize];

	size_t convertedChars = 0;
	mbstowcs_s(&convertedChars, wcstring, newsize, str.c_str(), _TRUNCATE);

	return wcstring;
}

void Log(const std::string& text)
{
	static std::ofstream file(GetCurrentDir() + "/BeyondApi/Plugins/ImprintingMod/logs.txt", std::ios_base::app);

	auto time = std::chrono::system_clock::now();
	std::time_t tTime = std::chrono::system_clock::to_time_t(time);

	char buffer[256];
	ctime_s(buffer, sizeof(buffer), &tTime);

	buffer[strlen(buffer) - 1] = '\0';

	std::string timeStr = buffer;

	std::string finalText = timeStr + ": " + text + "\n";

	file << finalText;

	file.flush();
}

std::string GetCurrentDir()
{
	char buffer[MAX_PATH];
	GetModuleFileNameA(nullptr, buffer, MAX_PATH);
	std::string::size_type pos = std::string(buffer).find_last_of("\\/");

	return std::string(buffer).substr(0, pos);
}