// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DSAIEditor : ModuleRules
{
	public DSAIEditor(TargetInfo Target)
	{
		
		PublicIncludePaths.AddRange(
			new string[] {
                "DSAIEditor/Public"
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
                "DSAIEditor/Private",
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
                "AnimGraph",
                "AnimGraphRuntime",
                "BlueprintGraph",
                "Core",
                "DSAIRuntime",
                "Kismet",
				// ... add other public dependencies that you statically link with here ...
			}
			);
			
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"CoreUObject",
				"Engine",
                "PropertyEditor",
                "Slate",
				"SlateCore",
                "UnrealEd",
				// ... add private dependencies that you statically link with here ...	
			}
			);
		
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
