using UnrealBuildTool;

public class MaterialInstanceGenerator : ModuleRules
{
	public MaterialInstanceGenerator(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		// Normally no special public include paths are required for this plugin.
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here if needed ...
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here if needed ...
			}
		);

		// Keep runtime/public dependencies minimal
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"Projects",
			}
		);

		// Editor-only and Slate/UI modules should go into private dependencies
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Slate",
				"SlateCore",
				"InputCore",
				"EditorStyle",
				"LevelEditor",
				"WorkspaceMenuStructure",
				"AssetTools",
				"AssetRegistry",
				"UnrealEd",
				"DesktopPlatform",
				"ToolMenus",
				"ContentBrowser",
				"EditorWidgets",
				"Json",
				"JsonUtilities",
				"PhysicsCore"
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
