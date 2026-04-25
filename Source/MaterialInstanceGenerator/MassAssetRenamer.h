#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "Widgets/Views/SHeaderRow.h"
#include "AssetData.h"

class SEditableTextBox;
class SScrollBox;
class STextBlock;
class SCheckBox;

struct FRenamePreviewItem
{
    FAssetData Asset;
    FString NewName;
    bool bValid;
};

class SMassAssetRenamerWindow : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SMassAssetRenamerWindow) {}
    SLATE_END_ARGS()

    void Construct(const FArguments& InArgs);

private:
    FString SourceFolder;
    bool bIncludeSubfolders = true;
    FString Prefix;
    FString Suffix;
    FString SearchPattern;
    FString ReplacePattern;
    bool bUseRegex = false;

    TArray<TSharedPtr<FRenamePreviewItem>> PreviewItems;
    TSharedPtr<SListView<TSharedPtr<FRenamePreviewItem>>> PreviewListView;
    TSharedPtr<SScrollBox> LogPanel;
    TSharedPtr<SVerticalBox> LogPanelBox;

    TSharedPtr<SEditableTextBox> SourceFolderTextBox;
    TSharedPtr<SCheckBox> SubfoldersCheckBox;
    TSharedPtr<SEditableTextBox> PrefixTextBox;
    TSharedPtr<SEditableTextBox> SuffixTextBox;
    TSharedPtr<SEditableTextBox> SearchTextBox;
    TSharedPtr<SEditableTextBox> ReplaceTextBox;
    TSharedPtr<SCheckBox> RegexCheckBox;
    TSharedPtr<STextBlock> StatusTextBlock;

    FReply OnSourceFolderBrowse();
    FReply OnRefreshPreview();
    FReply OnApplyRename();
    void OnSubfoldersChanged(ECheckBoxState NewState);
    void OnRegexChanged(ECheckBoxState NewState);

    void PickFolder(FString& OutPath, TSharedPtr<SEditableTextBox> TextBox, const FString& DialogTitle);
    void AddLog(const FString& Message, bool bIsError = false);
    void ClearLog();

    void GeneratePreview();
    FString ComputeNewName(const FString& OriginalName) const;
    TArray<FAssetData> GetAssetsInFolder(const FString& FolderPath, bool bRecursive) const;

    TSharedRef<ITableRow> OnGenerateRowForPreview(TSharedPtr<FRenamePreviewItem> Item, const TSharedRef<STableViewBase>& OwnerTable);
};