// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class DSAIEditor : ModuleRules
{
	public DSAIEditor(ReadOnlyTargetRules Target) : base(Target)
	{
        PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange (new string [] {
            // "DSAIEditor/Public"
            Path.Combine(ModuleDirectory, "Public")

			// ... add public include paths required here ...
		});
				
		
		PrivateIncludePaths.AddRange (new string [] {
            // "DSAIEditor/Private",
            Path.Combine(ModuleDirectory, "Private")
			// ... add other private include paths required here ...
		});
			
		
		PublicDependencyModuleNames.AddRange (new string [] {
            "AnimGraphRuntime",
            "Core",
            "DSAIRuntime",
			// ... add other public dependencies that you statically link with here ...
		});
			
		
		PrivateDependencyModuleNames.AddRange (new string [] {
			"CoreUObject",
			"Engine",
            "Slate",
			"SlateCore"
			// ... add private dependencies that you statically link with here ...	
		});
		
		
		DynamicallyLoadedModuleNames.AddRange (new string [] {
			// ... add any modules that your module loads dynamically here ...
		});

        if (Target.Type == TargetType.Editor) {
            PublicDependencyModuleNames.AddRange (new string [] {
                "AnimGraph",
                "BlueprintGraph",
                "Kismet"
            });
            PrivateDependencyModuleNames.AddRange (new string [] {
                "PropertyEditor",
                "UnrealEd"
            });
        }
    }
}
