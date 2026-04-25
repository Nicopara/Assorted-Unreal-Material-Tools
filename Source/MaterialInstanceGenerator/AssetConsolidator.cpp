#include "AssetConsolidator.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Input/SComboBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "AssetRegistryModule.h"
#include "IAssetTools.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Animation/AnimationAsset.h"
#include "Engine/Blueprint.h"
#include "EditorDirectories.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/Package.h"
#include "ScopedTransaction.h"
#include "Algo/Find.h"
#include "Internationalization/Regex.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"

#define LOCTEXT_NAMESPACE "AssetConsolidator"

void SAssetConsolidatorWindow::Construct(const FArguments& InArgs)
{
    LogPanelBox = SNew(SVerticalBox);
    AddLog("Ready.");

    ChildSlot
    [
        SNew(SScrollBox)
        + SScrollBox::Slot()
        [
            SNew(SVerticalBox)

            // Warning banner
            + SVerticalBox::Slot().AutoHeight().Padding(10.0f)
            [
                SNew(STextBlock)
                .Text(LOCTEXT("ConsolidationWarning", "WARNING: This tool permanently replaces asset references and can DELETE assets. Always backup your project before consolidating. Use with extreme caution!"))
                .ColorAndOpacity(FLinearColor(1.0f, 0.6f, 0.0f))
                .AutoWrapText(true)
                .Justification(ETextJustify::Center)
            ]

            // Source folder selection
            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [
                    SAssignNew(SourceFolderTextBox, SEditableTextBox)
                    .HintText(LOCTEXT("SourceFolderHint", "/Game/Path/To/Folder (leave empty for whole project)"))
                    .OnTextChanged_Lambda([this](const FText& InText) { SourceFolder = InText.ToString(); })
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton).Text(LOCTEXT("Browse", "Browse")).OnClicked(this, &SAssetConsolidatorWindow::OnSourceFolderBrowse)
                ]
            ]

            // Include subfolders
            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SAssignNew(SubfoldersCheckBox, SCheckBox)
                .IsChecked(ECheckBoxState::Checked)
                .OnCheckStateChanged(this, &SAssetConsolidatorWindow::OnSubfoldersChanged)
                .Content()[ SNew(STextBlock).Text(LOCTEXT("IncludeSubfolders", "Include subfolders")) ]
            ]

            // Asset type filters
            + SVerticalBox::Slot().AutoHeight().Padding(5, 10)
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                [
                    SNew(SVerticalBox)
                    + SVerticalBox::Slot().AutoHeight().Padding(5)
                    [
                        SNew(STextBlock).Text(LOCTEXT("AssetTypesHeader", "Asset types to scan:"))
                    ]
                    + SVerticalBox::Slot().AutoHeight().Padding(5)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().Padding(5)
                        [
                            SAssignNew(TextureCheckBox, SCheckBox)
                            .IsChecked(ECheckBoxState::Checked)
                            .OnCheckStateChanged(this, &SAssetConsolidatorWindow::OnFilterChanged)
                            .Content()[ SNew(STextBlock).Text(LOCTEXT("Textures", "Textures")) ]
                        ]
                        + SHorizontalBox::Slot().AutoWidth().Padding(5)
                        [
                            SAssignNew(MaterialCheckBox, SCheckBox)
                            .IsChecked(ECheckBoxState::Checked)
                            .OnCheckStateChanged(this, &SAssetConsolidatorWindow::OnFilterChanged)
                            .Content()[ SNew(STextBlock).Text(LOCTEXT("Materials", "Materials")) ]
                        ]
                        + SHorizontalBox::Slot().AutoWidth().Padding(5)
                        [
                            SAssignNew(MICheckBox, SCheckBox)
                            .IsChecked(ECheckBoxState::Checked)
                            .OnCheckStateChanged(this, &SAssetConsolidatorWindow::OnFilterChanged)
                            .Content()[ SNew(STextBlock).Text(LOCTEXT("MaterialInstances", "Material Instances")) ]
                        ]
                        + SHorizontalBox::Slot().AutoWidth().Padding(5)
                        [
                            SAssignNew(StaticMeshCheckBox, SCheckBox)
                            .IsChecked(ECheckBoxState::Checked)
                            .OnCheckStateChanged(this, &SAssetConsolidatorWindow::OnFilterChanged)
                            .Content()[ SNew(STextBlock).Text(LOCTEXT("StaticMeshes", "Static Meshes")) ]
                        ]
                        + SHorizontalBox::Slot().AutoWidth().Padding(5)
                        [
                            SAssignNew(SkeletalMeshCheckBox, SCheckBox)
                            .IsChecked(ECheckBoxState::Unchecked)
                            .OnCheckStateChanged(this, &SAssetConsolidatorWindow::OnFilterChanged)
                            .Content()[ SNew(STextBlock).Text(LOCTEXT("SkeletalMeshes", "Skeletal Meshes")) ]
                        ]
                        + SHorizontalBox::Slot().AutoWidth().Padding(5)
                        [
                            SAssignNew(AnimationCheckBox, SCheckBox)
                            .IsChecked(ECheckBoxState::Unchecked)
                            .OnCheckStateChanged(this, &SAssetConsolidatorWindow::OnFilterChanged)
                            .Content()[ SNew(STextBlock).Text(LOCTEXT("Animations", "Animations")) ]
                        ]
                        + SHorizontalBox::Slot().AutoWidth().Padding(5)
                        [
                            SAssignNew(BlueprintCheckBox, SCheckBox)
                            .IsChecked(ECheckBoxState::Unchecked)
                            .OnCheckStateChanged(this, &SAssetConsolidatorWindow::OnFilterChanged)
                            .Content()[ SNew(STextBlock).Text(LOCTEXT("Blueprints", "Blueprints")) ]
                        ]
                    ]
                ]
            ]

            // Delete duplicates option
            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SAssignNew(DeleteCheckBox, SCheckBox)
                .IsChecked(ECheckBoxState::Checked)
                .OnCheckStateChanged(this, &SAssetConsolidatorWindow::OnDeleteChanged)
                .Content()[ SNew(STextBlock).Text(LOCTEXT("DeleteDuplicates", "Delete duplicate assets after consolidation")) ]
            ]

            // Buttons
            + SVerticalBox::Slot().AutoHeight().Padding(5, 10)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(2)
                [
                    SNew(SButton).Text(LOCTEXT("Scan", "Scan for Duplicates")).OnClicked(this, &SAssetConsolidatorWindow::OnScanClicked)
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(2)
                [
                    SNew(SButton).Text(LOCTEXT("Consolidate", "Consolidate Selected")).OnClicked(this, &SAssetConsolidatorWindow::OnConsolidateClicked)
                ]
            ]

            // Preview list of duplicate groups
            + SVerticalBox::Slot().FillHeight(0.6f).Padding(5)
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                [
                    SAssignNew(PreviewListView, SListView<TSharedPtr<FConsolidationPreviewItem>>)
                    .ListItemsSource(&PreviewItems)
                    .OnGenerateRow(this, &SAssetConsolidatorWindow::OnGenerateRowForPreview)
                    .HeaderRow
                    (
                        SNew(SHeaderRow)
                        + SHeaderRow::Column("NormalizedName").DefaultLabel(LOCTEXT("NormalizedName", "Base Name")).FillWidth(0.15f)
                        + SHeaderRow::Column("Keep").DefaultLabel(LOCTEXT("Keep", "Keep Asset")).FillWidth(0.25f)
                        + SHeaderRow::Column("Duplicates").DefaultLabel(LOCTEXT("Duplicates", "Duplicate Assets (click to find)")).FillWidth(0.6f)
                    )
                ]
            ]

            // Log output (no inner scroll)
            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                [
                    LogPanelBox.ToSharedRef()
                ]
            ]
        ]
    ];
}

// ---------- Helpers ----------
void SAssetConsolidatorWindow::PickFolder(FString& OutPath, TSharedPtr<SEditableTextBox> TextBox, const FString& DialogTitle)
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
                AddLog("Please select a folder inside the project's Content directory.", true);
            }
        }
    }
}

FReply SAssetConsolidatorWindow::OnSourceFolderBrowse()
{
    PickFolder(SourceFolder, SourceFolderTextBox, "Select Folder (empty = whole project)");
    return FReply::Handled();
}

void SAssetConsolidatorWindow::OnSubfoldersChanged(ECheckBoxState NewState)
{
    bIncludeSubfolders = (NewState == ECheckBoxState::Checked);
}

void SAssetConsolidatorWindow::OnDeleteChanged(ECheckBoxState NewState)
{
    bDeleteDuplicates = (NewState == ECheckBoxState::Checked);
}

void SAssetConsolidatorWindow::OnFilterChanged(ECheckBoxState NewState)
{
    bIncludeTextures = (TextureCheckBox->GetCheckedState() == ECheckBoxState::Checked);
    bIncludeMaterials = (MaterialCheckBox->GetCheckedState() == ECheckBoxState::Checked);
    bIncludeMaterialInstances = (MICheckBox->GetCheckedState() == ECheckBoxState::Checked);
    bIncludeStaticMeshes = (StaticMeshCheckBox->GetCheckedState() == ECheckBoxState::Checked);
    bIncludeSkeletalMeshes = (SkeletalMeshCheckBox->GetCheckedState() == ECheckBoxState::Checked);
    bIncludeAnimations = (AnimationCheckBox->GetCheckedState() == ECheckBoxState::Checked);
    bIncludeBlueprint = (BlueprintCheckBox->GetCheckedState() == ECheckBoxState::Checked);
}

// ---------- Logging ----------
void SAssetConsolidatorWindow::AddLog(const FString& Message, bool bIsError)
{
    FString Prefix = bIsError ? TEXT("[ERROR] ") : TEXT("[INFO] ");
    FString Final = Prefix + Message;
    TSharedPtr<STextBlock> LogEntry = SNew(STextBlock)
        .Text(FText::FromString(Final))
        .ColorAndOpacity(bIsError ? FSlateColor(FLinearColor::Red) : FSlateColor(FLinearColor::White))
        .AutoWrapText(true);
    LogPanelBox->AddSlot().AutoHeight().Padding(2)[ LogEntry.ToSharedRef() ];
    UE_LOG(LogTemp, Log, TEXT("AssetConsolidator: %s"), *Final);
}

void SAssetConsolidatorWindow::ClearLog()
{
    LogPanelBox->ClearChildren();
    AddLog("Log cleared.");
}

// ---------- Normalization ----------
FString SAssetConsolidatorWindow::NormalizeAssetName(const FAssetData& Asset) const
{
    FString Name = Asset.AssetName.ToString().ToLower();

    // Remove copy/version/numeric suffixes only
    FRegexPattern Pattern(TEXT("_(copy|duplicate|dupe|v\\d+|version\\d+|\\d+)$"));
    FRegexMatcher Matcher(Pattern, Name);
    if (Matcher.FindNext())
    {
        int32 Start = Matcher.GetMatchBeginning();
        Name = Name.Left(Start);
    }
    return Name;
}

// ---------- Asset Type Helpers ----------
TArray<UClass*> SAssetConsolidatorWindow::GetActiveAssetClasses() const
{
    TArray<UClass*> Classes;
    if (bIncludeTextures) Classes.Add(UTexture2D::StaticClass());
    if (bIncludeMaterials) Classes.Add(UMaterial::StaticClass());
    if (bIncludeMaterialInstances) Classes.Add(UMaterialInstanceConstant::StaticClass());
    if (bIncludeStaticMeshes) Classes.Add(UStaticMesh::StaticClass());
    if (bIncludeSkeletalMeshes) Classes.Add(USkeletalMesh::StaticClass());
    if (bIncludeAnimations) Classes.Add(UAnimationAsset::StaticClass());
    if (bIncludeBlueprint) Classes.Add(UBlueprint::StaticClass());
    return Classes;
}

FString SAssetConsolidatorWindow::GetAssetTypeFriendlyName(UClass* Class) const
{
    if (Class == UTexture2D::StaticClass()) return TEXT("Texture");
    if (Class == UMaterial::StaticClass()) return TEXT("Material");
    if (Class == UMaterialInstanceConstant::StaticClass()) return TEXT("MaterialInstance");
    if (Class == UStaticMesh::StaticClass()) return TEXT("StaticMesh");
    if (Class == USkeletalMesh::StaticClass()) return TEXT("SkeletalMesh");
    if (Class->IsChildOf(UAnimationAsset::StaticClass())) return TEXT("Animation");
    if (Class == UBlueprint::StaticClass()) return TEXT("Blueprint");
    return Class->GetName();
}

// ---------- Scan ----------
void SAssetConsolidatorWindow::ScanForDuplicates()
{
    Groups.Empty();
    PreviewItems.Empty();

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<UClass*> AssetClasses = GetActiveAssetClasses();

    if (AssetClasses.Num() == 0)
    {
        AddLog("No asset types selected. Enable at least one checkbox.", true);
        PreviewListView->RequestListRefresh();
        return;
    }

    TMap<FString, TSharedPtr<FConsolidationGroup>> NormalizedGroups;

    for (UClass* Class : AssetClasses)
    {
        TArray<FAssetData> Assets;
        
        if (SourceFolder.IsEmpty())
        {
            AssetRegistryModule.Get().GetAssetsByClass(Class->GetFName(), Assets, true);
            int32 TotalFound = Assets.Num();
            Assets.RemoveAll([](const FAssetData& Asset)
            {
                return !Asset.PackageName.ToString().StartsWith(TEXT("/Game/"));
            });
            AddLog(FString::Printf(TEXT("Class %s: found %d total assets, %d under /Game/."),
                *Class->GetName(), TotalFound, Assets.Num()));
        }
        else
        {
            AssetRegistryModule.Get().GetAssetsByPath(FName(*SourceFolder), Assets, bIncludeSubfolders);
            Assets.RemoveAll([Class](const FAssetData& Asset){ return Asset.GetClass() != Class; });
            AddLog(FString::Printf(TEXT("Class %s: found %d assets in folder %s."),
                *Class->GetName(), Assets.Num(), *SourceFolder));
        }

        if (Assets.Num() == 0) continue;

        for (const FAssetData& Asset : Assets)
        {
            FString BaseName = NormalizeAssetName(Asset);
            FString Key = BaseName + TEXT("|") + Class->GetName();
            TSharedPtr<FConsolidationGroup>* ExistingGroup = NormalizedGroups.Find(Key);
            if (!ExistingGroup)
            {
                TSharedPtr<FConsolidationGroup> NewGroup = MakeShared<FConsolidationGroup>();
                NewGroup->NormalizedName = BaseName;
                NewGroup->KeepIndex = -1;
                NewGroup->bConsolidate = true;
                NormalizedGroups.Add(Key, NewGroup);
                ExistingGroup = NormalizedGroups.Find(Key);
            }
            (*ExistingGroup)->AllAssets.Add(Asset);
            (*ExistingGroup)->AssetsByClass.FindOrAdd(Class).Add(Asset);
        }
    }

    // Build duplicate groups and preview items
    for (auto& Pair : NormalizedGroups)
    {
        TSharedPtr<FConsolidationGroup> Group = Pair.Value;
        if (Group->AllAssets.Num() <= 1) continue;

        Group->KeepIndex = 0;
        Groups.Add(Group);

        TSharedPtr<FConsolidationPreviewItem> Item = MakeShared<FConsolidationPreviewItem>();
        Item->NormalizedName = Group->NormalizedName;
        Item->Group = Group;
        PreviewItems.Add(Item);

        // Log full paths of duplicates for this group
        AddLog(FString::Printf(TEXT("Duplicate group: '%s' (%d assets):"), *Group->NormalizedName, Group->AllAssets.Num()));
        for (const FAssetData& A : Group->AllAssets)
        {
            AddLog(FString::Printf(TEXT("    %s"), *A.PackageName.ToString()));
        }
    }

    int32 TotalAssetsInGroups = 0;
    for (const auto& G : Groups)
        TotalAssetsInGroups += G->AllAssets.Num();

    AddLog(FString::Printf(TEXT("Scan complete. Found %d duplicate groups containing %d assets."), Groups.Num(), TotalAssetsInGroups));
    PreviewListView->RequestListRefresh();
}

FReply SAssetConsolidatorWindow::OnScanClicked()
{
    ScanForDuplicates();
    return FReply::Handled();
}

// ---------- Redirect References ----------
void SAssetConsolidatorWindow::RedirectReferences(UObject* FromAsset, UObject* ToAsset) const
{
    if (!FromAsset || !ToAsset) return;

    FAssetRegistryModule& AssetRegistry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FName> Referencers;
    AssetRegistry.Get().GetReferencers(FromAsset->GetOutermost()->GetFName(), Referencers, UE::AssetRegistry::EDependencyCategory::Manage);

    for (const FName& ReferencerPkg : Referencers)
    {
        UPackage* Package = LoadPackage(nullptr, *ReferencerPkg.ToString(), LOAD_None);
        if (!Package) continue;
        Package->FullyLoad();
        TArray<UObject*> ObjectsInPackage;
        GetObjectsWithOuter(Package, ObjectsInPackage);

        for (UObject* Obj : ObjectsInPackage)
        {
            if (UMaterialInstanceConstant* MIC = Cast<UMaterialInstanceConstant>(Obj))
            {
                TArray<FMaterialParameterInfo> TexParams;
                TArray<FGuid> Guids;
                MIC->GetMaterial()->GetAllTextureParameterInfo(TexParams, Guids);
                for (const FMaterialParameterInfo& Param : TexParams)
                {
                    UTexture* CurrentTex = nullptr;
                    if (MIC->GetTextureParameterValue(Param, CurrentTex) && CurrentTex == FromAsset)
                    {
                        MIC->SetTextureParameterValueEditorOnly(Param, Cast<UTexture>(ToAsset));
                    }
                }
                if (MIC->Parent == FromAsset)
                {
                    MIC->SetParentEditorOnly(Cast<UMaterialInterface>(ToAsset));
                }
                MIC->PostEditChange();
                MIC->MarkPackageDirty();
            }
            else if (UStaticMesh* SM = Cast<UStaticMesh>(Obj))
            {
                TArray<FStaticMaterial> Materials = SM->GetStaticMaterials();
                for (FStaticMaterial& Mat : Materials)
                {
                    if (Mat.MaterialInterface == FromAsset)
                        Mat.MaterialInterface = Cast<UMaterialInterface>(ToAsset);
                }
                SM->SetStaticMaterials(Materials);
                SM->MarkPackageDirty();
            }
        }
    }
}

// ---------- Consolidation ----------
FReply SAssetConsolidatorWindow::OnConsolidateClicked()
{
    if (Groups.Num() == 0)
    {
        AddLog("No duplicate groups to consolidate. Run 'Scan for Duplicates' first.", true);
        return FReply::Handled();
    }

    FScopedTransaction Transaction(LOCTEXT("ConsolidateAssets", "Consolidate Assets"));
    int32 TotalRedirected = 0;
    int32 TotalDeleted = 0;

    for (const TSharedPtr<FConsolidationGroup>& Group : Groups)
    {
        if (!Group->bConsolidate) continue;
        if (Group->KeepIndex < 0 || Group->KeepIndex >= Group->AllAssets.Num())
        {
            AddLog(FString::Printf(TEXT("Invalid keep index for group '%s'. Skipping."), *Group->NormalizedName), true);
            continue;
        }

        UObject* KeepAsset = Group->AllAssets[Group->KeepIndex].GetAsset();
        if (!KeepAsset)
        {
            AddLog(FString::Printf(TEXT("Failed to load keep asset for group '%s'."), *Group->NormalizedName), true);
            continue;
        }

        TArray<FAssetData> Duplicates;
        for (int32 i = 0; i < Group->AllAssets.Num(); ++i)
        {
            if (i == Group->KeepIndex) continue;
            Duplicates.Add(Group->AllAssets[i]);
        }

        for (const FAssetData& Dup : Duplicates)
        {
            UObject* DupAsset = Dup.GetAsset();
            if (!DupAsset) continue;
            RedirectReferences(DupAsset, KeepAsset);
            TotalRedirected++;
            AddLog(FString::Printf(TEXT("Redirected references from %s to %s"), *Dup.AssetName.ToString(), *KeepAsset->GetName()));
        }

        if (bDeleteDuplicates)
        {
            DeleteAssets(Duplicates);
            TotalDeleted += Duplicates.Num();
            AddLog(FString::Printf(TEXT("Deleted %d duplicate assets for group '%s'"), Duplicates.Num(), *Group->NormalizedName));
        }
    }

    AddLog(FString::Printf(TEXT("Consolidation complete. Redirected %d references, deleted %d assets."), TotalRedirected, TotalDeleted));
    return FReply::Handled();
}

void SAssetConsolidatorWindow::DeleteAssets(const TArray<FAssetData>& AssetsToDelete) const
{
    if (AssetsToDelete.Num() == 0) return;
    
    TArray<FString> PackagesToDelete;
    for (const FAssetData& Asset : AssetsToDelete)
        PackagesToDelete.Add(Asset.PackageName.ToString());
    
    for (const FString& PackageName : PackagesToDelete)
    {
        TArray<UObject*> ObjectsToDelete;
        if (UPackage* Package = FindPackage(nullptr, *PackageName))
        {
            GetObjectsWithOuter(Package, ObjectsToDelete, false);
            ObjectsToDelete.Add(Package);
            for (UObject* Obj : ObjectsToDelete)
            {
                if (Obj)
                {
                    Obj->Modify();
                    Obj->ClearFlags(RF_Standalone | RF_Public);
                    Obj->SetFlags(RF_Transient);
                }
            }
            if (Package)
                Package->MarkPackageDirty();
        }
    }
}

// ---------- Sync to Content Browser ----------
void SAssetConsolidatorWindow::SyncContentBrowserToAsset(const FAssetData& Asset) const
{
    if (Asset.IsValid())
    {
        TArray<FAssetData> AssetsToSync;
        AssetsToSync.Add(Asset);
        FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
        ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync);
    }
}

// ---------- UI Row Generation ----------
TSharedRef<ITableRow> SAssetConsolidatorWindow::OnGenerateRowForPreview(
    TSharedPtr<FConsolidationPreviewItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    // Build options: full path + type
    Item->Options.Empty();
    for (int32 i = 0; i < Item->Group->AllAssets.Num(); ++i)
    {
        const FAssetData& Asset = Item->Group->AllAssets[i];
        FString Display = FString::Printf(TEXT("%s (%s)"),
            *Asset.PackageName.ToString(),
            *GetAssetTypeFriendlyName(Asset.GetClass()));
        Item->Options.Add(MakeShared<FString>(Display));
    }

    int32 SelectedIdx = FMath::Clamp(Item->Group->KeepIndex, 0, Item->Options.Num() - 1);
    Item->SelectedOption = Item->Options[SelectedIdx];

    TSharedPtr<SComboBox<TSharedPtr<FString>>> ComboBox;
    TSharedPtr<FConsolidationPreviewItem> ItemPtr = Item;

    // Build duplicate buttons (all except the keep index)
    TSharedRef<SHorizontalBox> DuplicateButtonsBox = SNew(SHorizontalBox);
    for (int32 i = 0; i < Item->Group->AllAssets.Num(); ++i)
    {
        if (i == Item->Group->KeepIndex) continue;
        const FAssetData& Dup = Item->Group->AllAssets[i];
        DuplicateButtonsBox->AddSlot()
        .AutoWidth()
        .Padding(2)
        [
            SNew(SButton)
            .Text(FText::FromString(Dup.PackageName.ToString()))
            .ToolTipText(FText::Format(LOCTEXT("FindAssetTooltip", "Find {0} in Content Browser"),
                FText::FromString(Dup.PackageName.ToString())))
            .OnClicked_Lambda([this, Dup]()
            {
                SyncContentBrowserToAsset(Dup);
                return FReply::Handled();
            })
        ];
    }

    return SNew(STableRow<TSharedPtr<FConsolidationPreviewItem>>, OwnerTable)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(0.15f).Padding(2).VAlign(VAlign_Center)
            [
                SNew(STextBlock).Text(FText::FromString(Item->NormalizedName))
            ]
            + SHorizontalBox::Slot().FillWidth(0.25f).Padding(2)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [
                    SAssignNew(ComboBox, SComboBox<TSharedPtr<FString>>)
                    .OptionsSource(&ItemPtr->Options)
                    .OnGenerateWidget_Lambda([](TSharedPtr<FString> InOption)
                    {
                        return SNew(STextBlock).Text(FText::FromString(InOption.IsValid() ? *InOption : TEXT("")));
                    })
                    .OnSelectionChanged_Lambda([this, ItemPtr](TSharedPtr<FString> NewSelection, ESelectInfo::Type)
                    {
                        if (NewSelection.IsValid())
                        {
                            ItemPtr->SelectedOption = NewSelection;
                            int32 NewIndex = ItemPtr->Options.Find(NewSelection);
                            if (NewIndex != INDEX_NONE && ItemPtr->Group.IsValid())
                            {
                                ItemPtr->Group->KeepIndex = NewIndex;
                            }
                            // Refresh to update duplicate buttons
                            if (PreviewListView.IsValid())
                                PreviewListView->RequestListRefresh();
                        }
                    })
                    .Content()
                    [
                        SNew(STextBlock)
                        .Text_Lambda([ItemPtr]()
                        {
                            return ItemPtr.IsValid() && ItemPtr->SelectedOption.IsValid()
                                ? FText::FromString(*ItemPtr->SelectedOption)
                                : FText::GetEmpty();
                        })
                    ]
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(2,0,0,0)
                [
                    SNew(SButton)
                    .Text(LOCTEXT("FindKeep", "Find"))
                    .ToolTipText(LOCTEXT("FindKeepTooltip", "Find the selected keep asset in Content Browser"))
                    .OnClicked_Lambda([this, ItemPtr]()
                    {
                        if (ItemPtr.IsValid() && ItemPtr->Group.IsValid() &&
                            ItemPtr->Group->KeepIndex >= 0 && ItemPtr->Group->KeepIndex < ItemPtr->Group->AllAssets.Num())
                        {
                            SyncContentBrowserToAsset(ItemPtr->Group->AllAssets[ItemPtr->Group->KeepIndex]);
                        }
                        return FReply::Handled();
                    })
                ]
            ]
            + SHorizontalBox::Slot().FillWidth(0.6f).Padding(2)
            [
                DuplicateButtonsBox
            ]
        ];
}

#undef LOCTEXT_NAMESPACE