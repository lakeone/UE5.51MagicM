// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class IntelOIDN : ModuleRules
{
    public IntelOIDN(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        if (Target.Platform.IsInGroup(UnrealPlatformGroup.Windows) && Target.Architecture.bIsX64)
        {
			string SDKDir         = Target.UEThirdPartySourceDirectory + "Intel/OIDN/";
			string Embree3_SDKDir = Target.UEThirdPartySourceDirectory + "Intel/Embree/Embree3122/Win64/";

			PublicSystemIncludePaths.Add(SDKDir + "include/");
            PublicSystemLibraryPaths.Add(SDKDir + "lib/");
            PublicAdditionalLibraries.Add(SDKDir + "lib/OpenImageDenoise.lib");
			PublicAdditionalLibraries.Add(SDKDir + "lib/OpenImageDenoise_core.lib");
			RuntimeDependencies.Add("$(TargetOutputDir)/OpenImageDenoise.dll"           , SDKDir + "bin/OpenImageDenoise.dll");
			RuntimeDependencies.Add("$(TargetOutputDir)/OpenImageDenoise_core.dll"      , SDKDir + "bin/OpenImageDenoise_core.dll");
			RuntimeDependencies.Add("$(TargetOutputDir)/OpenImageDenoise_device_cpu.dll", SDKDir + "bin/OpenImageDenoise_device_cpu.dll");

			//@todo - find a cleaner way to share this library with Embree3
			//RuntimeDependencies.Add("$(TargetOutputDir)/tbb12.dll", SDKDir + "bin/tbb12.dll");
			RuntimeDependencies.Add("$(TargetOutputDir)/tbb12.dll", Embree3_SDKDir + "lib/tbb12.dll");
			PublicDelayLoadDLLs.Add("OpenImageDenoise.dll");
			PublicDelayLoadDLLs.Add("OpenImageDenoise_core.dll");
			PublicDelayLoadDLLs.Add("OpenImageDenoise_device_cpu.dll");
			PublicDelayLoadDLLs.Add("tbb12.dll");
			PublicDefinitions.Add("WITH_INTELOIDN=1");
        }
		else
		{
			PublicDefinitions.Add("WITH_INTELOIDN=0");
		}
    }
}
