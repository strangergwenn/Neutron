// Neutron - Gwennaël Arbona

using UnrealBuildTool;
using System.IO;

public class Neutron : ModuleRules
{
	public Neutron(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Niagara",

			"Slate",
			"SlateCore",
			"UMG",
			"InputCore",

			"Json",
			"JsonUtilities",

			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"NetCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"EngineSettings",
			"AppFramework",
			"RHI",
			"RenderCore",
			"MoviePlayer"
		});

		if (Target.Type == TargetType.Editor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] {
				"UnrealEd"
			});
        }

        if (Target.Platform.IsInGroup(UnrealPlatformGroup.Windows))
        {
            PrivateDependencyModuleNames.AddRange(new string[] {
                "DLSS"
            });
        }
    }
}
