#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "AssetData.h"

class SEditableTextBox;
class SScrollBox;
class STextBlock;
class SCheckBox;
class SVerticalBox;
template<typename ItemType> class SListView;

struct FConsolidationGroup;

struct FConsolidationPreviewItem
{
    FString NormalizedName;
    TSharedPtr<FConsolidationGroup> Group;
    FString KeepAssetName;
    FString DuplicateNames;

    TArray<TSharedPtr<FString>> Options;
    // Currently selected option (persists across row refreshes)
    TSharedPtr<FString> SelectedOption;
};

struct FConsolidationGroup
{
    FString NormalizedName;
    int32 KeepIndex = -1;
    bool bConsolidate = true;
    TArray<FAssetData> AllAssets;
    TMap<UClass*, TArray<FAssetData>> AssetsByClass;
};

class SAssetConsolidatorWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SAssetConsolidatorWindow) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    TSharedPtr<SEditableTextBox> SourceFolderTextBox;
    TSharedPtr<SCheckBox> SubfoldersCheckBox;
    TSharedPtr<SCheckBox> TextureCheckBox;
    TSharedPtr<SCheckBox> MaterialCheckBox;
    TSharedPtr<SCheckBox> MICheckBox;
    TSharedPtr<SCheckBox> StaticMeshCheckBox;
    TSharedPtr<SCheckBox> SkeletalMeshCheckBox;
    TSharedPtr<SCheckBox> AnimationCheckBox;
    TSharedPtr<SCheckBox> BlueprintCheckBox;
    TSharedPtr<SCheckBox> DeleteCheckBox;

    TSharedPtr<SScrollBox> LogPanel;
    TSharedPtr<SVerticalBox> LogPanelBox;

    TSharedPtr<SListView<TSharedPtr<FConsolidationPreviewItem>>> PreviewListView;
    TArray<TSharedPtr<FConsolidationPreviewItem>> PreviewItems;
    TArray<TSharedPtr<FConsolidationGroup>> Groups;

    FString SourceFolder;
    bool bIncludeSubfolders = true;
    bool bIncludeTextures = true;
    bool bIncludeMaterials = true;
    bool bIncludeMaterialInstances = true;
    bool bIncludeStaticMeshes = true;
    bool bIncludeSkeletalMeshes = false;
    bool bIncludeAnimations = false;
    bool bIncludeBlueprint = false;
    bool bDeleteDuplicates = true;

    FReply OnSourceFolderBrowse();
    void OnSubfoldersChanged(ECheckBoxState NewState);
    void OnDeleteChanged(ECheckBoxState NewState);
    void OnFilterChanged(ECheckBoxState NewState);

    void PickFolder(FString& OutPath, TSharedPtr<SEditableTextBox> TextBox, const FString& DialogTitle);

    void AddLog(const FString& Message, bool bIsError = false);
    void ClearLog();

    FString NormalizeAssetName(const FAssetData& Asset) const;
    TArray<UClass*> GetActiveAssetClasses() const;
    FString GetAssetTypeFriendlyName(UClass* Class) const;

    void ScanForDuplicates();
    FReply OnScanClicked();

    void RedirectReferences(UObject* FromAsset, UObject* ToAsset) const;
    FReply OnConsolidateClicked();
    void DeleteAssets(const TArray<FAssetData>& AssetsToDelete) const;

    void SyncContentBrowserToAsset(const FAssetData& Asset) const;

    TSharedRef<ITableRow> OnGenerateRowForPreview(TSharedPtr<FConsolidationPreviewItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
};