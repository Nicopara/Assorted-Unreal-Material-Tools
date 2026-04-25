#include "AssortedMaterialToolsModule.h"
#include "MaterialInstanceGeneratorWindow.h"
#include "PhysicalMaterialAssigner.h"
#include "ModelMaterialApplier.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "ToolMenus.h"
#include "MassAssetRenamer.h"
#include "AssetConsolidator.h"

#define LOCTEXT_NAMESPACE "FAssortedMaterialToolsModule"

void FAssortedMaterialToolsModule::StartupModule()
{
    
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner("AssetConsolidator", FOnSpawnTab::CreateRaw(this, &FAssortedMaterialToolsModule::OnSpawnConsolidatorTab))
    .SetDisplayName(LOCTEXT("ConsolidatorTabTitle", "Asset Consolidator"))
    .SetMenuType(ETabSpawnerMenuType::Hidden);
    
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner("MassAssetRenamer", FOnSpawnTab::CreateRaw(this, &FAssortedMaterialToolsModule::OnSpawnRenamerTab))
    .SetDisplayName(LOCTEXT("RenamerTabTitle", "Mass Asset Renamer"))
    .SetMenuType(ETabSpawnerMenuType::Hidden);
    
    // Register Generator tab
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner("MaterialInstanceGenerator", FOnSpawnTab::CreateRaw(this, &FAssortedMaterialToolsModule::OnSpawnGeneratorTab))
        .SetDisplayName(LOCTEXT("GeneratorTabTitle", "Material Generator"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    // Register Physical Material Assigner tab
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner("PhysicalMaterialAssigner", FOnSpawnTab::CreateRaw(this, &FAssortedMaterialToolsModule::OnSpawnAssignerTab))
        .SetDisplayName(LOCTEXT("AssignerTabTitle", "Physical Material Assigner"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);

    // Register Model Material Applier tab
    FGlobalTabmanager::Get()->RegisterNomadTabSpawner("ModelMaterialApplier", FOnSpawnTab::CreateRaw(this, &FAssortedMaterialToolsModule::OnSpawnApplierTab))
        .SetDisplayName(LOCTEXT("ApplierTabTitle", "Model Material Applier"))
        .SetMenuType(ETabSpawnerMenuType::Hidden);
    RegisterMenus();
}

void FAssortedMaterialToolsModule::ShutdownModule()
{
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("AssetConsolidator");
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("MaterialInstanceGenerator");
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("PhysicalMaterialAssigner");
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("ModelMaterialApplier");
    FGlobalTabmanager::Get()->UnregisterNomadTabSpawner("MassAssetRenamer");
    UToolMenus::UnregisterOwner(this);
}

TSharedRef<SDockTab> FAssortedMaterialToolsModule::OnSpawnGeneratorTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab).TabRole(ETabRole::NomadTab)[ SNew(SMaterialInstanceGeneratorWindow) ];
}

TSharedRef<SDockTab> FAssortedMaterialToolsModule::OnSpawnAssignerTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab).TabRole(ETabRole::NomadTab)[ SNew(SPhysicalMaterialAssignerWindow) ];
}

TSharedRef<SDockTab> FAssortedMaterialToolsModule::OnSpawnApplierTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab).TabRole(ETabRole::NomadTab)[ SNew(SModelMaterialApplierWindow) ];
}

TSharedRef<SDockTab> FAssortedMaterialToolsModule::OnSpawnRenamerTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab).TabRole(ETabRole::NomadTab)[ SNew(SMassAssetRenamerWindow) ];
}

TSharedRef<SDockTab> FAssortedMaterialToolsModule::OnSpawnConsolidatorTab(const FSpawnTabArgs& SpawnTabArgs)
{
    return SNew(SDockTab).TabRole(ETabRole::NomadTab)[ SNew(SAssetConsolidatorWindow) ];
}

void FAssortedMaterialToolsModule::RegisterMenus()
{
    
    UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
    FToolMenuSection& Section = Menu->FindOrAddSection("WindowLayout");

    Section.AddMenuEntry("OpenAssetConsolidator",
        LOCTEXT("OpenConsolidatorLabel", "Asset Consolidator"),
        LOCTEXT("OpenConsolidatorTooltip", "Consolidate duplicate assets by redirecting references"),
    FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FAssortedMaterialToolsModule::OpenConsolidatorWindow)));
    
    Section.AddMenuEntry("OpenMassAssetRenamer",
        LOCTEXT("OpenRenamerLabel", "Mass Asset Renamer"),
        LOCTEXT("OpenRenamerTooltip", "Bulk rename assets in a folder using patterns"),
    FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FAssortedMaterialToolsModule::OpenRenamerWindow)));
    
    Section.AddMenuEntry("OpenMaterialInstanceGenerator",
        LOCTEXT("OpenGeneratorLabel", "Material Instance Generator"),
        LOCTEXT("OpenGeneratorTooltip", "Creates material instances from texture folders"),
        FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FAssortedMaterialToolsModule::OpenGeneratorWindow)));

    Section.AddMenuEntry("OpenPhysicalMaterialAssigner",
        LOCTEXT("OpenAssignerLabel", "Physical Material Assigner"),
        LOCTEXT("OpenAssignerTooltip", "Assigns physical materials to material instances using WordNet"),
        FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FAssortedMaterialToolsModule::OpenAssignerWindow)));

    Section.AddMenuEntry("OpenModelMaterialApplier",
        LOCTEXT("OpenApplierLabel", "Model Material Applier"),
        LOCTEXT("OpenApplierTooltip", "Applies material instances to static meshes"),
        FSlateIcon(), FUIAction(FExecuteAction::CreateRaw(this, &FAssortedMaterialToolsModule::OpenApplierWindow)));
}

void FAssortedMaterialToolsModule::OpenGeneratorWindow() { FGlobalTabmanager::Get()->TryInvokeTab(FName("MaterialInstanceGenerator")); }
void FAssortedMaterialToolsModule::OpenAssignerWindow() { FGlobalTabmanager::Get()->TryInvokeTab(FName("PhysicalMaterialAssigner")); }
void FAssortedMaterialToolsModule::OpenApplierWindow() { FGlobalTabmanager::Get()->TryInvokeTab(FName("ModelMaterialApplier")); }
void FAssortedMaterialToolsModule::OpenRenamerWindow() { FGlobalTabmanager::Get()->TryInvokeTab(FName("MassAssetRenamer")); }
void FAssortedMaterialToolsModule::OpenConsolidatorWindow() { FGlobalTabmanager::Get()->TryInvokeTab(FName("AssetConsolidator")); }

#undef LOCTEXT_NAMESPACE
IMPLEMENT_MODULE(FAssortedMaterialToolsModule, MaterialInstanceGenerator)