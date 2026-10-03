// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MassDemo : ModuleRules
{
	public MassDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"Niagara",
			"UMG",
			"Slate",
			"MassCore",
			"MassEntity",
			"MassCommon",
			"MassSignals",
			"MassActors",
			"MassSpawner",
			"MassRepresentation",
			"MassMovement",
			"MassNavigation",
			"StructUtils"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"MassDemo",
			"MassDemo/Variant_Strategy",
			"MassDemo/Variant_Strategy/UI",
			"MassDemo/Variant_TwinStick",
			"MassDemo/Variant_TwinStick/AI",
			"MassDemo/Variant_TwinStick/Gameplay",
			"MassDemo/Variant_TwinStick/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
