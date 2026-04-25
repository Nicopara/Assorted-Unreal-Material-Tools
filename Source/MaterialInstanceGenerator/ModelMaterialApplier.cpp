#include "ModelMaterialApplier.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "AssetRegistryModule.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceConstant.h"
#include "EditorDirectories.h"

#define LOCTEXT_NAMESPACE "ModelMaterialApplier"

void SModelMaterialApplierWindow::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SScrollBox)
        + SScrollBox::Slot()
        [
            SNew(SVerticalBox)

            // Source Models folder
            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [
                    SAssignNew(SourceModelsTextBox, SEditableTextBox)
                    .HintText(LOCTEXT("SourceModelsHint", "/Game/Path/To/SourceModels"))
                    .OnTextChanged_Lambda([this](const FText& InText) { SourceModelsFolder = InText.ToString(); })
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton).Text(LOCTEXT("BrowseSrc", "Browse")).OnClicked(this, &SModelMaterialApplierWindow::OnSourceModelsBrowse)
                ]
            ]

            // Target Models folder (can be same as source)
            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [
                    SAssignNew(TargetModelsTextBox, SEditableTextBox)
                    .HintText(LOCTEXT("TargetModelsHint", "/Game/Path/To/TargetModels (or same as source)"))
                    .OnTextChanged_Lambda([this](const FText& InText) { TargetModelsFolder = InText.ToString(); })
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton).Text(LOCTEXT("BrowseTarget", "Browse")).OnClicked(this, &SModelMaterialApplierWindow::OnTargetModelsBrowse)
                ]
            ]

            // Material Instances folder
            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [
                    SAssignNew(MaterialInstancesTextBox, SEditableTextBox)
                    .HintText(LOCTEXT("MIFolderHint", "/Game/Path/To/MaterialInstances"))
                    .OnTextChanged_Lambda([this](const FText& InText) { MaterialInstancesFolder = InText.ToString(); })
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton).Text(LOCTEXT("BrowseMI", "Browse")).OnClicked(this, &SModelMaterialApplierWindow::OnMaterialInstancesBrowse)
                ]
            ]

            + SVerticalBox::Slot().AutoHeight().Padding(5, 20, 5, 5)
            [
                SNew(SButton)
                .Text(LOCTEXT("ApplyBtn", "Apply Material Instances to Models"))
                .HAlign(HAlign_Center)
                .OnClicked(this, &SModelMaterialApplierWindow::OnApplyClicked)
            ]

            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SAssignNew(StatusTextBlock, STextBlock)
                .Text(LOCTEXT("Ready", "Ready."))
            ]
        ]
    ];
}

// ---------- Pickers ----------
void SModelMaterialApplierWindow::PickFolder(FString& OutPath, TSharedPtr<SEditableTextBox> TextBox, const FString& DialogTitle)
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform)
    {
        FString PickedPath;
        if (DesktopPlatform->OpenDirectoryDialog(nullptr, DialogTitle, FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_IMPORT), PickedPath))
        {
            FString ContentDir = FPaths::ProjectContentDir();
            if (PickedPath.StartsWith(ContentDir))
            {
                OutPath = PickedPath.Replace(*ContentDir, TEXT("/Game/"));
                TextBox->SetText(FText::FromString(OutPath));
            }
            else
            {
                UpdateStatus("Please select a folder inside the project's Content directory.", true);
            }
        }
    }
}

FReply SModelMaterialApplierWindow::OnSourceModelsBrowse()
{
    PickFolder(SourceModelsFolder, SourceModelsTextBox, "Select Source Models Folder");
    return FReply::Handled();
}

FReply SModelMaterialApplierWindow::OnTargetModelsBrowse()
{
    PickFolder(TargetModelsFolder, TargetModelsTextBox, "Select Target Models Folder");
    return FReply::Handled();
}

FReply SModelMaterialApplierWindow::OnMaterialInstancesBrowse()
{
    PickFolder(MaterialInstancesFolder, MaterialInstancesTextBox, "Select Material Instances Folder");
    return FReply::Handled();
}

FReply SModelMaterialApplierWindow::OnApplyClicked()
{
    ApplyMaterialInstancesToModels();
    return FReply::Handled();
}

// ---------- Core Logic ----------
void SModelMaterialApplierWindow::ApplyMaterialInstancesToModels()
{
    if (SourceModelsFolder.IsEmpty() || TargetModelsFolder.IsEmpty() || MaterialInstancesFolder.IsEmpty())
    {
        UpdateStatus("Error: Source Models, Target Models and Material Instances folders are required.", true);
        return;
    }

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    // Load all material instances from the MI folder
    TArray<FAssetData> MIList;
    AssetRegistryModule.Get().GetAssetsByPath(FName(*MaterialInstancesFolder), MIList, true);
    TMap<FString, UMaterialInstanceConstant*> MIMap;
    for (const FAssetData& MIData : MIList)
    {
        if (UMaterialInstanceConstant* MI = Cast<UMaterialInstanceConstant>(MIData.GetAsset()))
        {
            FString Name = MI->GetName();
            if (Name.StartsWith(TEXT("MI_"))) Name = Name.RightChop(3);
            MIMap.Add(Name, MI);
        }
    }

    if (MIMap.Num() == 0)
    {
        UpdateStatus("No Material Instances found in the specified folder.", true);
        return;
    }

    // Load all meshes from source folder
    TArray<FAssetData> MeshAssets;
    AssetRegistryModule.Get().GetAssetsByPath(FName(*SourceModelsFolder), MeshAssets, true);

    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    int32 ProcessedMeshes = 0;

    for (const FAssetData& MeshData : MeshAssets)
    {
        UStaticMesh* SourceMesh = Cast<UStaticMesh>(MeshData.GetAsset());
        if (!SourceMesh) continue;

        FString MeshName = SourceMesh->GetName();
        // Try exact match first, then trim common prefixes
        UMaterialInstanceConstant** FoundMI = MIMap.Find(MeshName);
        if (!FoundMI)
        {
            FString CleanMeshName = MeshName.Replace(TEXT("_SM"), TEXT("")).Replace(TEXT("_Static"), TEXT(""));
            FoundMI = MIMap.Find(CleanMeshName);
            if (!FoundMI) continue;
        }

        UMaterialInstanceConstant* MI = *FoundMI;

        UStaticMesh* TargetMesh = SourceMesh;
        if (SourceModelsFolder != TargetModelsFolder)
        {
            // Duplicate asset to target folder
            UObject* DuplicatedMesh = AssetTools.DuplicateAsset(MeshName, TargetModelsFolder, SourceMesh);
            TargetMesh = Cast<UStaticMesh>(DuplicatedMesh);
            if (!TargetMesh)
            {
                UpdateStatus(FString::Printf(TEXT("Failed to duplicate mesh %s"), *MeshName), true);
                continue;
            }
        }

        // Assign the material instance to all material slots (UE 4.27 compatible)
        TArray<FStaticMaterial> Materials = TargetMesh->GetStaticMaterials();
        for (FStaticMaterial& Mat : Materials)
        {
            Mat.MaterialInterface = MI;
        }
        TargetMesh->SetStaticMaterials(Materials);

        TargetMesh->MarkPackageDirty();
        ProcessedMeshes++;
    }

    UpdateStatus(FString::Printf(TEXT("Applied material instances to %d meshes."), ProcessedMeshes));
}

void SModelMaterialApplierWindow::UpdateStatus(const FString& Message, bool bIsError)
{
    StatusTextBlock->SetText(FText::FromString(Message));
    if (bIsError)
    {
        UE_LOG(LogTemp, Warning, TEXT("ModelMaterialApplier: %s"), *Message);
    }
    else
        UE_LOG(LogTemp, Log, TEXT("ModelMaterialApplier: %s"), *Message);
}

#undef LOCTEXT_NAMESPACE