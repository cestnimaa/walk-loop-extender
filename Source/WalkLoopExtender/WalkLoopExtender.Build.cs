using UnrealBuildTool;

public class WalkLoopExtender : ModuleRules
{
    public WalkLoopExtender(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "CoreUObject",
                "Engine"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "AnimationDataController",
                "AssetRegistry",
                "AssetTools",
                "ContentBrowser",
                "DeveloperSettings",
                "Slate",
                "SlateCore",
                "ToolMenus",
                "UnrealEd"
            }
        );
    }
}
