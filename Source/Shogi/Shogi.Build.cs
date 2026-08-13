// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class Shogi : ModuleRules
{
	public Shogi(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Source files live under feature subfolders (Core/Gameplay/GameModes/UI) rather
		// than flat in the module root; register each as an include path so plain
		// #include "Foo.h" keeps resolving regardless of engine include-order settings.
		PublicIncludePaths.AddRange(new string[]
		{
			ModuleDirectory + "/Core",
			ModuleDirectory + "/Gameplay",
			ModuleDirectory + "/GameModes",
			ModuleDirectory + "/UI",
		});
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
