Tool Details
1. Mass Asset Renamer

Features

    Add a prefix and/or suffix to all assets in a folder (and optionally subfolders).

    Perform search & replace on asset names.

    Regular expression mode for advanced pattern matching.

    Live preview table showing original name, new name, and status.

    Apply the rename with one click.

How to use

    Set the source folder (e.g., /Game/Textures).

    Enable/disable Include subfolders.

    Fill in Prefix, Suffix, Search text, and Replace text.

    Click Refresh Preview to see affected assets.

    Click Apply Rename.

2. Material Instance Generator

Features

    Parses a master material to detect texture parameters.

    For each parameter you can assign a folder containing textures.

    The first non‑empty folder drives instance creation (e.g., BaseColor folder).

    Automatically creates MI_+texture name instances and sets all matching textures by scanning the assigned folders.

    Supports custom parameter names and folder mapping.

How to use

    Master Material – browse to your base material (.uasset).

    Output Folder – where generated MI_* assets will be saved.

    Click Scan Parameters – populates the texture parameter list.

    For each parameter, either:

        Leave folder empty → skip.

        Assign a folder → the generator will look for textures that match the base name (cleaned).

    The driver parameter (e.g., BaseColor) determines the naming of new instances.

    Click Generate Material Instances.

3. Physical Material Assigner

Features

    Scans all material instances in a folder.

    Scans all UPhysicalMaterial assets in a physical materials folder.

    Uses a keyword matching algorithm to find the most suitable physical material based on the material instance name (removes MI_ prefix).

    Keywords include common material types: wood, metal, stone, plastic, glass, fabric, rubber, concrete, dirt, sand, ice, water, grass, leather, bone, flesh, paper, cloth, gravel, mud, snow, brick, ceramic, carpet, tile, blood, hell, meat, alien, rock, marble, asphalt, clay, rust.

    Falls back to tokenising the name if no keyword matches.

How to use

    Material Instances folder – where your MI_* assets are.

    Physical Materials folder – where your UPhysicalMaterial assets are stored (e.g., /Game/Physics/PhysicalMaterials).

    Click Assign Physical Materials.

    The tool prints a log of assignments and skips.

4. Model Material Applier

Features

    Copies material instance assignments from source static meshes to target static meshes.

    Matches mesh names to material instances (stripping SM_ and _Static common prefixes).

    Can duplicate the mesh into a target folder (keeping the original unchanged).

    Assigns the matched material instance to every material slot of the target mesh.

How to use

    Source Models folder – contains the original meshes that already have material slots (or not).

    Target Models folder – where processed meshes will be saved (can be the same as source).

    Material Instances folder – contains the MI_* assets to assign.

    Click Apply Material Instances to Models.

    The log will show which meshes were processed.

    Note: The tool applies the material instance to all slots of the mesh. For fine‑grained control, consider using the standard Unreal material editor.

5. Asset Consolidator

Features

    Scans a folder (or entire /Game/) for duplicate assets by normalized name (ignores _copy, _v2, numeric suffixes, etc.).

    Groups duplicates by asset type (textures, materials, material instances, static meshes, skeletal meshes, animations, blueprints).

    For each group, you can choose which asset to keep via a dropdown.

    All references from the duplicates are redirected to the kept asset.

    Optionally deletes the duplicate assets after consolidation.

    Logs full path of every duplicate group before consolidation.

How to use

    (Optional) Set a source folder – leave empty to scan the whole /Game/.

    Enable Include subfolders if desired.

    Check the asset types you want to scan.

    Click Scan for Duplicates – the preview list shows each duplicate group.

    For each group, select the asset to keep from the dropdown.

    Click Consolidate Selected – all references are updated and duplicates may be deleted according to the Delete duplicate assets checkbox.

    ⚠️ WARNING: This tool performs irreversible changes. Always backup your project before consolidating. The tool provides a clear warning banner in the UI.

Building and Integration

If you want to use the AssortedMaterialToolsModule as a standalone plugin:

    Place the module in Plugins/AssortedMaterialTools/Source/AssortedMaterialTools/.

    Add the necessary AssortedMaterialTools.uplugin file with "Type": "Editor".

    Enable the plugin in your project.

The module automatically registers menu entries under LevelEditor.MainMenu.Window. If you prefer not to use this module, simply include the individual tool windows in your own editor module – each tool is a SCompoundWidget ready to be used in a SDockTab.
License

These tools are provided as‑is, free to use and modify in any Unreal Engine project, no attribution required. They are not officially supported by Epic Games. Use at your own risk.
Credits

Developed as a collection of productivity helpers for asset and material workflows in Unreal Engine 4.26+ and 5.x.
