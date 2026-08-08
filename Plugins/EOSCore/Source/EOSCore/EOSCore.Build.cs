/**
* Copyright (C) 2017-2025 | eelDev AB
*
* EOSCore Documentation: https://eeldev.com
*/

using UnrealBuildTool;

public class EOSCore : ModuleRules
{
	public EOSCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		//PrivatePCHHeaderFile = "Private/Core/EOSCorePluginPrivatePCH.h";
       
        PublicDependencyModuleNames.AddRange(
			new string[]
                {
                    "Core",
                    "EOSLibrary",
                    "Projects",
                    "OnlineSubsystemEOSCore"
                }
            );

        PrivateDependencyModuleNames.AddRange(
            new string[]
                {
                    "Engine",
                    "CoreUObject",
                    "OnlineSubsystem",
                    "OnlineSubsystemUtils",
                    "OnlineSubsystemEOSCore",
                    "EOSCoreShared",
                }
            );
    }
}
