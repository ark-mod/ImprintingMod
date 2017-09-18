#include <windows.h>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>
#include <cmath>
#include "API/Base.h"
#include "Tools.h"
#include "ImprintingMod.h"

#pragma comment(lib, "ArkApi.lib")

nlohmann::json json;
sqlite::database* db;
bool bExit = false;
std::chrono::time_point<std::chrono::system_clock> NextCleanupTime;

void LoadConfig();

void Imprint(AShooterPlayerController* aShooterPlayerController, FString* message, int mode)
{
	UWorld* world = Ark::GetWorld();
	if (!world) return;
	if (!aShooterPlayerController) return;

	ACharacter* character = aShooterPlayerController->GetCharacterField();
	if (!character || !character->IsA(APrimalCharacter::GetPrivateStaticClass())) return;

	std::stringstream ss;

	APrimalCharacter* primalCharacter = static_cast<APrimalCharacter*>(character);
	AActor* actor = primalCharacter->GetAimedActor(ECC_GameTraceChannel2, 0i64, 0.0, 0.0, 0i64, 0i64, 0, 0);
	if (actor && actor->IsA(APrimalDinoCharacter::GetPrivateStaticClass())) {
		APrimalDinoCharacter* dino = static_cast<APrimalDinoCharacter*>(actor);
		int dinoTeam = dino->GetTargetingTeamField();
		int playerTeam = aShooterPlayerController->GetTargetingTeamField();
		if (dinoTeam == playerTeam)
		{
			float babyAge = dino->GetBabyAgeField();
			if (babyAge < 1.0f) {
				long double time = world->GetTimeSecondsField();
				long double prevTime = dino->GetBabyNextCuddleTimeField();
				if (time < prevTime) {
					bool error = false;
					bool noSpeciesFound = false;
					long long dinoId1 = dino->GetDinoID1Field();
					long long dinoId2 = dino->GetDinoID2Field();
					int limit = 0; //default limit

					//get species specific limits
					FString* className = new FString();
					FName bp = actor->GetClassField()->GetNameField();
					bp.ToString(className);
					//dino->GetDinoNameTagField().ToString(className); //species name

					auto species = json["Species"];

					auto speciesIter = species.find(className->ToString());
					if (speciesIter != species.end())
					{
						double oneThird = 1.0 / 3.0;
						double twoThirds = oneThird * 2.0;
						double maturationTimeDefault = speciesIter.value();
						double babyMatureSpeedMultiplier = json["BabyMatureSpeedMultiplier"];
						double days = (maturationTimeDefault / babyMatureSpeedMultiplier) / 86400.0;
						int wholeDays = (int)std::floor(days);
						double partialDay = days - wholeDays;
						limit = wholeDays * 2 + (partialDay > oneThird ? (partialDay > twoThirds ? 2 : 1) : 0);

						//int days = (int)std::round((maturationTimeDefault / babyMatureSpeedMultiplier) / 86400.0);
						//limit = 2 * days;
					}
					else noSpeciesFound = true;

					delete className;

					int count = 0;
					try
					{
						*db << "SELECT Id,At,Id1,Id2,Num FROM Imprints WHERE Id1 = ? AND Id2 = ? LIMIT 1;" << dinoId1 << dinoId2 >>
							[&](long long id, long long at, long long id1, long long id2, int num) {
							count = num;
						};
					}
					catch (sqlite::sqlite_exception& e)
					{
						std::cout << "Unexpected DB error (1) " << e.what() << std::endl;
						error = true;
					}

					if (error) ss << "Unexpected error";
					else if (noSpeciesFound) ss << "No advance cuddles are allowed for this species.";
					else if (limit == 0) ss << "No advance cuddles are allowed for this species due to short maturation time.";
					else if (count >= limit) ss << "This creature have reached the advance cuddle limit of " << limit << " for this species.";
					else {
						try
						{
							if (count == 0) {
								*db << u"INSERT INTO Imprints (At,Id1,Id2,Num) VALUES (?,?,?,?);"
									<< std::chrono::system_clock::now().time_since_epoch().count() << dinoId1 << dinoId2 << 1;
							}
							else {
								*db << u"UPDATE Imprints SET Num = ? WHERE Id1 = ? AND Id2 = ?;"
									<< (count + 1) << dinoId1 << dinoId2;
							}
						}
						catch (sqlite::sqlite_exception& e)
						{
							std::cout << "Unexpected DB error (2) " << e.what() << std::endl;
							error = true;
						}

						if (error) ss << "Unexpected error (2)";
						else {
							TSubclassOf<UPrimalItem> cuddleFood = dino->GetBabyCuddleFoodField();
							//EBabyCuddleType::Type cuddleType = dino->GetBabyCuddleTypeField();
							dino->UpdateBabyCuddling(time, EBabyCuddleType::Type::Food, cuddleFood); //immediate cuddle, always using food, food type is randomized by the game
							ss << "Advance cuddle unlocked (" << (limit - count - 1) << "/" << limit << " left).";
						}
					}
				}
				else ss << "Cuddle is already available for target creature.";
			}
			else ss << "Target is not a baby creature";
		}
		else if (dinoTeam < 50000) ss << "Target is not a tamed creature.";
		else ss << "Target does not belong to you or your team.";
	} 
	else ss << "Target is not a creature.";

	wchar_t* wcstring = ConvertToWideStr(ss.str());
	SendChatMessage(aShooterPlayerController, L"[imprint system]", wcstring);

	delete[] wcstring;
}

void ImprintCheck(AShooterPlayerController* aShooterPlayerController, FString* message, int mode)
{
	UWorld* world = Ark::GetWorld();
	if (!world) return;
	if (!aShooterPlayerController) return;

	ACharacter* character = aShooterPlayerController->GetCharacterField();
	if (!character || !character->IsA(APrimalCharacter::GetPrivateStaticClass())) return;

	std::stringstream ss;

	APrimalCharacter* primalCharacter = static_cast<APrimalCharacter*>(character);
	AActor* actor = primalCharacter->GetAimedActor(ECC_GameTraceChannel2, 0i64, 0.0, 0.0, 0i64, 0i64, 0, 0);
	if (actor && actor->IsA(APrimalDinoCharacter::GetPrivateStaticClass())) {
		APrimalDinoCharacter* dino = static_cast<APrimalDinoCharacter*>(actor);
		int dinoTeam = dino->GetTargetingTeamField();
		int playerTeam = aShooterPlayerController->GetTargetingTeamField();
		if (dinoTeam == playerTeam)
		{
			bool error = false;
			bool noSpeciesFound = false;
			long long dinoId1 = dino->GetDinoID1Field();
			long long dinoId2 = dino->GetDinoID2Field();
			int limit = 0; //default limit

			//get species specific limits
			FString* className = new FString();
			FName bp = actor->GetClassField()->GetNameField();
			bp.ToString(className);
			//dino->GetDinoNameTagField().ToString(className); //species name

			auto species = json["Species"];

			auto speciesIter = species.find(className->ToString());
			if (speciesIter != species.end())
			{
				double oneThird = 1.0 / 3.0;
				double twoThirds = oneThird * 2.0;
				double maturationTimeDefault = speciesIter.value();
				double babyMatureSpeedMultiplier = json["BabyMatureSpeedMultiplier"];
				double days = (maturationTimeDefault / babyMatureSpeedMultiplier) / 86400.0;
				int wholeDays = (int)std::floor(days);
				double partialDay = days - wholeDays;
				limit = wholeDays * 2 + (partialDay > oneThird ? (partialDay > twoThirds ? 2 : 1) : 0);

				//int days = (int)std::round((maturationTimeDefault / babyMatureSpeedMultiplier) / 86400.0);
				//limit = 2 * days;
			}
			else noSpeciesFound = true;

			delete className;

			int count = 0;
			try
			{
				*db << "SELECT Id,At,Id1,Id2,Num FROM Imprints WHERE Id1 = ? AND Id2 = ? LIMIT 1;" << dinoId1 << dinoId2 >>
					[&](long long id, long long at, long long id1, long long id2, int num) {
					count = num;
				};
			}
			catch (sqlite::sqlite_exception& e)
			{
				std::cout << "Unexpected DB error (1) " << e.what() << std::endl;
				error = true;
			}

			float babyAge = dino->GetBabyAgeField();

			if (error) ss << "Unexpected error";
			else if (noSpeciesFound) ss << "No advance cuddles are allowed for this species.";
			else if (limit == 0) ss << "No advance cuddles are allowed for this species due to short maturation time.";
			else if (count >= limit) ss << "This creature have reached the advance cuddle limit of " << limit << " for this species.";
			else if (babyAge >= 1.0f) ss << "Advance cuddle limit is " << limit << " for this species.";
			else ss << "Advance cuddle check (" << (limit - count) << "/" << limit << " left).";
		}
		else if (dinoTeam < 50000) ss << "Target is not a tamed creature.";
		else ss << "Target does not belong to you or your team.";
	}
	else ss << "Target is not a creature.";

	wchar_t* wcstring = ConvertToWideStr(ss.str());
	SendChatMessage(aShooterPlayerController, L"[imprint system]", wcstring); //todo: replace this with a screen message print preferably

	delete[] wcstring;
}

void CleanupTimer()
{
	auto now = std::chrono::system_clock::now();
	auto diff = std::chrono::duration_cast<std::chrono::seconds>(NextCleanupTime - now);

	if (diff.count() <= 0)
	{
		NextCleanupTime = now + std::chrono::seconds(3600);

		try
		{
			*db << u"DELETE FROM Imprints WHERE At <= ?;" << (std::chrono::system_clock::now() - std::chrono::seconds(3600 * 24 * 7)).time_since_epoch().count();
		}
		catch (sqlite::sqlite_exception& e)
		{
			std::cout << "Unexpected DB error (3) " << e.what() << std::endl;
		}
	}
}

void Init()
{
	if (sqlite3_threadsafe() == 0) {
		Log("sqlite is not threadsafe");
		throw;
	}

	LoadConfig();

	std::string databasePath = json["DatabasePath"];
	if (databasePath.empty()) {
		Log("DatabasePath must be set in config.json");
		throw;
	}
	//todo: mkdir here

	double babyMatureSpeedMultiplier = json["BabyMatureSpeedMultiplier"];
	if (babyMatureSpeedMultiplier <= 0.0) {
		Log("BabyMatureSpeedMultiplier must be set in config.json");
		throw;
	}

	NextCleanupTime = std::chrono::system_clock::now() + std::chrono::seconds(300); //init with cleanup in 5 min

	db = new sqlite::database(databasePath);
	*db << u"create table if not exists Imprints ("
		"Id integer primary key autoincrement not null,"
		"At integer default 0,"
		"Id1 integer default 0,"
		"Id2 integer default 0,"
		"Num integer default 0"
		");";

	Ark::AddChatCommand(L"/imprint", &Imprint);
	Ark::AddChatCommand(L"/imprintcheck", &ImprintCheck);

	Ark::AddOnTimerCallback(&CleanupTimer);
}

void LoadConfig()
{
	std::ifstream file(GetCurrentDir() + "/BeyondApi/Plugins/ImprintingMod/config.json");
	if (!file.is_open())
	{
		Log("Could not open file config.json");
		throw;
	}

	file >> json;
	file.close();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Init();
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		bExit = true;

		if (db) {
			delete db;
			db = NULL;
		}
		break;
	}
	return TRUE;
}