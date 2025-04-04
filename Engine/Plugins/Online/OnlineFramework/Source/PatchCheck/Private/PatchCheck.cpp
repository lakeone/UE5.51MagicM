// Copyright Epic Games, Inc. All Rights Reserved.

#include "PatchCheck.h"
#include "PatchCheckModule.h"

#include "Misc/App.h"
#include "Online.h"

DEFINE_LOG_CATEGORY(LogPatchCheck);

const TCHAR* LexToString(EPatchCheckResult Value)
{
	switch (Value)
	{
	case EPatchCheckResult::NoPatchRequired:	return TEXT("NoPatchRequired");
	case EPatchCheckResult::PatchRequired:		return TEXT("PatchRequired");
	case EPatchCheckResult::NoLoggedInUser:		return TEXT("NoLoggedInUser");
	default:									checkNoEntry(); // Intentional fallthrough
	case EPatchCheckResult::PatchCheckFailure:	return TEXT("PatchCheckFailure");
	}
}

FPatchCheck& FPatchCheck::Get()
{
	static IPatchCheckModule* ConfiguredModule = nullptr;

	if (ConfiguredModule && ConfiguredModule->GetPatchCheck())
	{
		return *ConfiguredModule->GetPatchCheck();
	}

	FString ModuleName;
	if (GConfig->GetString(TEXT("PatchCheck"), TEXT("ModuleName"), ModuleName, GEngineIni))
	{
		if (FModuleManager::Get().ModuleExists(*ModuleName))
		{
			ConfiguredModule = FModuleManager::LoadModulePtr<IPatchCheckModule>(*ModuleName);
		}

		if (ConfiguredModule && ConfiguredModule->GetPatchCheck())
		{
			return *ConfiguredModule->GetPatchCheck();
		}
		else
		{
			// Couldn't find the configured module, fallback to the default
			ensureMsgf(false, TEXT("FPatchCheck: Couldn't find module with Name %s, using default"), ModuleName.IsEmpty() ? TEXT("None") : *ModuleName);
		}
	}

	// No override module configured, use default
	ConfiguredModule = &FModuleManager::LoadModuleChecked<IPatchCheckModule>(TEXT("PatchCheck"));
	return *ConfiguredModule->MakePatchCheck();
}

FPatchCheck::~FPatchCheck()
{

}

FPatchCheck::FPatchCheck()
{
	RefreshConfig();
}

void FPatchCheck::RefreshConfig()
{
	if (!GConfig->GetBool(TEXT("PatchCheck"), TEXT("bCheckPlatformOSSForUpdate"), bCheckPlatformOSSForUpdate, GEngineIni))
	{
		/** For backwards compatibility with UUpdateManager */
		if (GConfig->GetBool(TEXT("/Script/Hotfix.UpdateManager"), TEXT("bCheckPlatformOSSForUpdate"), bCheckPlatformOSSForUpdate, GEngineIni))
		{
			ensureMsgf(false, TEXT("UpdateManager::bCheckPlatformOSSForUpdate is deprecated, Set FPatchCheck::bCheckPlatformOSSForUpdate using section [PatchCheck] instead."));
		}
	}

	if (!GConfig->GetBool(TEXT("PatchCheck"), TEXT("bCheckOSSForUpdate"), bCheckOSSForUpdate, GEngineIni))
	{
		/** For backwards compatibility with UUpdateManager */
		if (GConfig->GetBool(TEXT("/Script/Hotfix.UpdateManager"), TEXT("bCheckOSSForUpdate"), bCheckOSSForUpdate, GEngineIni))
		{
			ensureMsgf(false, TEXT("UpdateManager::bCheckOSSForUpdate is deprecated, Set FPatchCheck::bCheckOSSForUpdate using section [PatchCheck] instead."));
		}
	}

	GConfig->GetBool(TEXT("PatchCheck"), TEXT("bPlatformEnvironmentDetectionEnabled"), bPlatformEnvironmentDetectionEnabled, GEngineIni);
}

void FPatchCheck::OnDetectPlatformEnvironmentComplete(const FOnlineError& Result)
{
	if (Result.WasSuccessful())
	{
		bPlatformEnvironmentDetected = true;
		HandleOSSPatchCheck();
	}
	else
	{
		if (Result.GetErrorCode().Contains(TEXT("getUserAccessCode failed : 0x8055000f"), ESearchCase::IgnoreCase))
		{
			UE_LOG(LogPatchCheck, Warning, TEXT("Failed to complete login because patch is required"));
			PatchCheckComplete(EPatchCheckResult::PatchRequired);
		}
		else
		{
			if (Result.GetErrorCode().Contains(TEXT("com.epicgames.identity.notloggedin"), ESearchCase::IgnoreCase))
			{
				UE_LOG(LogPatchCheck, Warning, TEXT("Failed to detect online environment for the platform, no user signed in"));
				PatchCheckComplete(EPatchCheckResult::NoLoggedInUser);
			}
			else
			{
				// just a platform env error, assume production and keep going
				UE_LOG(LogPatchCheck, Warning, TEXT("Failed to detect online environment for the platform"));
				bPlatformEnvironmentDetected = true;
				HandleOSSPatchCheck();
			}
		}
	}
}

void FPatchCheck::StartPatchCheck()
{
	if (bIsCheckInProgress)
		return;

	RefreshConfig();

	if (bPlatformEnvironmentDetectionEnabled && !bPlatformEnvironmentDetected)
	{
#if PATCH_CHECK_PLATFORM_ENVIRONMENT_DETECTION
		if (DetectPlatformEnvironment())
		{
			return;
		}
#endif
	}

	HandleOSSPatchCheck();
}

void FPatchCheck::HandleOSSPatchCheck()
{
	if (bCheckPlatformOSSForUpdate && IOnlineSubsystem::GetByPlatform() != nullptr)
	{
		bIsCheckInProgress = true;
		StartPlatformOSSPatchCheck();
	}
	else if (bCheckOSSForUpdate)
	{
		bIsCheckInProgress = true;
		StartOSSPatchCheck();
	}
	else
	{
		UE_LOG(LogPatchCheck, Warning, TEXT("Patch check disabled for both Platform and Default OSS"));
	}
}

void FPatchCheck::AddEnvironmentWantsPatchCheckBackCompatDelegate(FName Tag, FEnvironmentWantsPatchCheck Delegate)
{
	BackCompatEnvironmentWantsPatchCheckDelegates.Emplace(Tag, MoveTemp(Delegate));
}

void FPatchCheck::RemoveEnvironmentWantsPatchCheckBackCompatDelegate(FName Tag)
{
	BackCompatEnvironmentWantsPatchCheckDelegates.Remove(Tag);
}

void FPatchCheck::StartPlatformOSSPatchCheck()
{
	EPatchCheckResult PatchResult = EPatchCheckResult::PatchCheckFailure;
	bool bStarted = false;

	IOnlineSubsystem* PlatformOnlineSub = IOnlineSubsystem::GetByPlatform();
	check(PlatformOnlineSub);
	IOnlineIdentityPtr PlatformOnlineIdentity = PlatformOnlineSub->GetIdentityInterface();
	if (PlatformOnlineIdentity.IsValid())
	{
		FUniqueNetIdPtr UserId = GetFirstSignedInUser(PlatformOnlineIdentity);
#if !PATCH_CHECK_PRIVILEGE_MUST_BE_LOGGED_IN
		// some platforms will log the user in if required in all but the NotLoggedIn state
		const bool bCanCheckPlayOnlinePrivilege = UserId.IsValid() && (PlatformOnlineIdentity->GetLoginStatus(*UserId) != ELoginStatus::NotLoggedIn);
#else
		const bool bCanCheckPlayOnlinePrivilege = UserId.IsValid() && (PlatformOnlineIdentity->GetLoginStatus(*UserId) == ELoginStatus::LoggedIn);
#endif
		if (bCanCheckPlayOnlinePrivilege)
		{
			bStarted = true;
			PlatformOnlineIdentity->GetUserPrivilege(*UserId,
				EUserPrivileges::CanPlayOnline, IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateRaw(this, &FPatchCheck::OnCheckForPatchComplete, true));
		}
		else
		{
			UE_LOG(LogPatchCheck, Warning, TEXT("No valid platform user id when starting patch check!"));
			PatchResult = EPatchCheckResult::NoLoggedInUser;
		}
	}

	if (!bStarted)
	{
		// Any failure to call GetUserPrivilege will result in completing the flow via this path
		PatchCheckComplete(PatchResult);
	}
}

void FPatchCheck::StartOSSPatchCheck()
{
	if (SkipPatchCheck())
	{
		UE_LOG(LogPatchCheck, Verbose, TEXT("[StartOSSPatchCheck] Skipping patch check."));

		// Trigger completion if check is skipped.
		PatchCheckComplete(EPatchCheckResult::NoPatchRequired);
	}
	else
	{
		EPatchCheckResult PatchResult = EPatchCheckResult::PatchCheckFailure;
		bool bStarted = false;

		// Online::GetIdentityInterface() can take a UWorld for correctness, but that only matters in PIE right now
		// and update checks should never happen in PIE currently.
		IOnlineIdentityPtr IdentityInt = Online::GetIdentityInterface();
		if (IdentityInt.IsValid())
		{
			// User could be invalid for "before title/login" check, underlying code doesn't need a valid user currently
			FUniqueNetIdPtr UserId = IdentityInt->CreateUniquePlayerId(TEXT("InvalidUser"));
			if (UserId.IsValid())
			{
				bStarted = true;
				IdentityInt->GetUserPrivilege(*UserId,
					EUserPrivileges::CanPlayOnline, IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate::CreateRaw(this, &FPatchCheck::OnCheckForPatchComplete, false));
			}
		}

		if (!bStarted)
		{
			// Any failure to call GetUserPrivilege will result in completing the flow via this path
			PatchCheckComplete(PatchResult);
		}
	}
}

bool FPatchCheck::EnvironmentWantsPatchCheck() const
{
	if (BackCompatEnvironmentWantsPatchCheckDelegates.Num() > 0)
	{
		for (const TPair<FName, FEnvironmentWantsPatchCheck>& Pair : BackCompatEnvironmentWantsPatchCheckDelegates)
		{
			if (Pair.Value.IsBound() && Pair.Value.Execute())
			{
				return true;
			}
		}
	}

	return false;
}

bool FPatchCheck::EditorWantsPatchCheck() const
{
	return false;
}

bool FPatchCheck::SkipPatchCheck() const
{
	// Does the environment care about patch checks (LIVE, STAGE, etc)
	const bool bEnvironmentWantsPatchCheck = EnvironmentWantsPatchCheck();

	// Can always opt in to a check
	const bool bForcePatchCheck = FParse::Param(FCommandLine::Get(), TEXT("ForcePatchCheck"));

	// Check whether editor needs a patch check
	const bool bEditorWantsPatchCheck = EditorWantsPatchCheck();
	const bool bSkipDueToEditor = UE_EDITOR && !bEditorWantsPatchCheck;

	// Prevent a patch check on dedicated server. UpdateManager also doesn't do a patch check on dedicated server.
	const bool bSkipDueToDedicatedServer = IsRunningDedicatedServer();

	// prevent a check when running unattended
	const bool bSkipDueToUnattended = FApp::IsUnattended();

	// Explicitly skipping the check
	const bool bForceSkipCheck = FParse::Param(FCommandLine::Get(), TEXT("SkipPatchCheck"));
	const bool bSkipPatchCheck = !bForcePatchCheck && (!bEnvironmentWantsPatchCheck || bSkipDueToEditor || bSkipDueToDedicatedServer || bForceSkipCheck || bSkipDueToUnattended);

	return bSkipPatchCheck;
}

inline EPatchCheckResult TranslatePatchCheckResult(uint32 PrivilegeResult)
{
	EPatchCheckResult Result = EPatchCheckResult::NoPatchRequired;

	if (PrivilegeResult & (uint32)IOnlineIdentity::EPrivilegeResults::RequiredSystemUpdate)
	{
		Result = EPatchCheckResult::PatchRequired;
	}
	else if (PrivilegeResult & (uint32)IOnlineIdentity::EPrivilegeResults::RequiredPatchAvailable)
	{
		Result = EPatchCheckResult::PatchRequired;
	}
	else if (PrivilegeResult & ((uint32)IOnlineIdentity::EPrivilegeResults::UserNotLoggedIn | (uint32)IOnlineIdentity::EPrivilegeResults::UserNotFound))
	{
		Result = EPatchCheckResult::NoLoggedInUser;
	}
	else if (PrivilegeResult & (uint32)IOnlineIdentity::EPrivilegeResults::GenericFailure)
	{
		CA_CONSTANT_IF(PATCH_CHECK_FAIL_ON_GENERIC_FAILURE)
		{
			Result = EPatchCheckResult::PatchCheckFailure;
		}
		else
		{
			// Skip console backend failures
			Result = EPatchCheckResult::NoPatchRequired;
		}
	}

	return Result;
}

void FPatchCheck::OnCheckForPatchComplete(const FUniqueNetId& UniqueId, EUserPrivileges::Type Privilege, uint32 PrivilegeResult, bool bConsoleCheck)
{
	const EPatchCheckResult Result = Privilege == EUserPrivileges::CanPlayOnline ? TranslatePatchCheckResult(PrivilegeResult) : EPatchCheckResult::NoPatchRequired;
	UE_LOG(LogPatchCheck, Verbose, TEXT("[OnCheckForPatchComplete] Type: %s, Privilege: %d, PrivilegeResult: %d, PatchCheckResult: %s"), bConsoleCheck ? TEXT("PlatformOSS") : TEXT("DefaultOSS"), (uint32)Privilege, PrivilegeResult, LexToString(Result));

	if (bCheckOSSForUpdate && bConsoleCheck && Result == EPatchCheckResult::NoPatchRequired)
	{
		// We perform both checks in this case
		StartOSSPatchCheck();
		return;
	}

	PatchCheckComplete(Result);
}

void FPatchCheck::PatchCheckComplete(EPatchCheckResult PatchResult)
{
	OnComplete.Broadcast(PatchResult);
	bIsCheckInProgress = false;
}
