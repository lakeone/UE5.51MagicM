// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Delegates/IDelegateInstance.h"
#include "Misc/Fork.h"
#include "Modules/ModuleInterface.h"

#include "EOSSDKManager.h"

class FEOSSharedModule: public IModuleInterface
{
public:
	FEOSSharedModule() = default;
	~FEOSSharedModule() = default;

	static FEOSSharedModule* Get();

	const TArray<FString>& GetSuppressedLogStrings() const { return SuppressedLogStrings; }
	const TArray<FString>& GetSuppressedLogCategories() const { return SuppressedLogCategories; }

private:
	// ~Begin IModuleInterface
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	// ~End IModuleInterface

	void OnConfigSectionsChanged(const FString& IniFilename, const TSet<FString>& SectionNames);
	void LoadConfig();


#if WITH_EOS_SDK
	TUniquePtr<FEOSSDKManager> SDKManager;
#endif
	TArray<FString> SuppressedLogStrings;
	TArray<FString> SuppressedLogCategories;
	FDelegateHandle OnPostForkDelegateHandle;
};