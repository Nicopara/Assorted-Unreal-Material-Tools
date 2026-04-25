#include "MassAssetRenamer.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Views/SListView.h"
#include "Widgets/Views/STableRow.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "AssetRegistryModule.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "EditorDirectories.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Internationalization/Regex.h"

#define LOCTEXT_NAMESPACE "MassAssetRenamer"

void SMassAssetRenamerWindow::Construct(const FArguments& InArgs)
{
    // Initialize log panel
    LogPanelBox = SNew(SVerticalBox);
    AddLog("Ready.");

    ChildSlot
    [
        SNew(SScrollBox)
        + SScrollBox::Slot()
        [
            SNew(SVerticalBox)

            // Source folder selection
            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [
                    SAssignNew(SourceFolderTextBox, SEditableTextBox)
                    .HintText(LOCTEXT("SourceFolderHint", "/Game/Path/To/Folder"))
                    .OnTextChanged_Lambda([this](const FText& InText) { SourceFolder = InText.ToString(); })
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton).Text(LOCTEXT("Browse", "Browse")).OnClicked(this, &SMassAssetRenamerWindow::OnSourceFolderBrowse)
                ]
            ]

            // Include subfolders
            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SAssignNew(SubfoldersCheckBox, SCheckBox)
                .IsChecked(ECheckBoxState::Checked)
                .OnCheckStateChanged(this, &SMassAssetRenamerWindow::OnSubfoldersChanged)
                .Content()[ SNew(STextBlock).Text(LOCTEXT("IncludeSubfolders", "Include subfolders")) ]
            ]

            // Rename rules group
            + SVerticalBox::Slot().AutoHeight().Padding(5, 10, 5, 5)
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                [
                    SNew(SVerticalBox)

                    + SVerticalBox::Slot().AutoHeight().Padding(5)
                    [
                        SNew(STextBlock).Text(LOCTEXT("RulesHeader", "Rename Rules"))
                    ]

                    + SVerticalBox::Slot().AutoHeight().Padding(5)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().Padding(2)[ SNew(STextBlock).Text(LOCTEXT("Prefix", "Prefix:")) ]
                        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)[ SAssignNew(PrefixTextBox, SEditableTextBox) ]
                    ]

                    + SVerticalBox::Slot().AutoHeight().Padding(5)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().Padding(2)[ SNew(STextBlock).Text(LOCTEXT("Suffix", "Suffix:")) ]
                        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)[ SAssignNew(SuffixTextBox, SEditableTextBox) ]
                    ]

                    + SVerticalBox::Slot().AutoHeight().Padding(5)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().Padding(2)[ SNew(STextBlock).Text(LOCTEXT("Search", "Search:")) ]
                        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)[ SAssignNew(SearchTextBox, SEditableTextBox) ]
                    ]

                    + SVerticalBox::Slot().AutoHeight().Padding(5)
                    [
                        SNew(SHorizontalBox)
                        + SHorizontalBox::Slot().AutoWidth().Padding(2)[ SNew(STextBlock).Text(LOCTEXT("Replace", "Replace:")) ]
                        + SHorizontalBox::Slot().FillWidth(1.0f).Padding(2)[ SAssignNew(ReplaceTextBox, SEditableTextBox) ]
                    ]

                    + SVerticalBox::Slot().AutoHeight().Padding(5)
                    [
                        SAssignNew(RegexCheckBox, SCheckBox)
                        .IsChecked(ECheckBoxState::Unchecked)
                        .OnCheckStateChanged(this, &SMassAssetRenamerWindow::OnRegexChanged)
                        .Content()[ SNew(STextBlock).Text(LOCTEXT("UseRegex", "Use Regular Expressions")) ]
                    ]
                ]
            ]

            // Buttons
            + SVerticalBox::Slot().AutoHeight().Padding(5, 10)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().AutoWidth().Padding(2)
                [
                    SNew(SButton).Text(LOCTEXT("Preview", "Refresh Preview")).OnClicked(this, &SMassAssetRenamerWindow::OnRefreshPreview)
                ]
                + SHorizontalBox::Slot().AutoWidth().Padding(2)
                [
                    SNew(SButton).Text(LOCTEXT("Apply", "Apply Rename")).OnClicked(this, &SMassAssetRenamerWindow::OnApplyRename)
                ]
            ]

            // Preview list
            + SVerticalBox::Slot().FillHeight(0.6f).Padding(5)
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                [
                    SAssignNew(PreviewListView, SListView<TSharedPtr<FRenamePreviewItem>>)
                    .ListItemsSource(&PreviewItems)
                    .OnGenerateRow(this, &SMassAssetRenamerWindow::OnGenerateRowForPreview)
                    .HeaderRow
                    (
                        SNew(SHeaderRow)
                        + SHeaderRow::Column("OriginalName").DefaultLabel(LOCTEXT("Original", "Original Name")).FillWidth(0.4f)
                        + SHeaderRow::Column("NewName").DefaultLabel(LOCTEXT("New", "New Name")).FillWidth(0.4f)
                        + SHeaderRow::Column("Status").DefaultLabel(LOCTEXT("Status", "Status")).FillWidth(0.2f)
                    )
                ]
            ]

            // Log output
            + SVerticalBox::Slot().FillHeight(0.3f).Padding(5)
            [
                SNew(SBorder)
                .BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
                [
                    SAssignNew(LogPanel, SScrollBox)
                    + SScrollBox::Slot()
                    [
                        LogPanelBox.ToSharedRef()
                    ]
                ]
            ]
        ]
    ];
}

// ---------- Callbacks ----------
void SMassAssetRenamerWindow::PickFolder(FString& OutPath, TSharedPtr<SEditableTextBox> TextBox, const FString& DialogTitle)
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

FReply SMassAssetRenamerWindow::OnSourceFolderBrowse()
{
    PickFolder(SourceFolder, SourceFolderTextBox, "Select Folder to Rename Assets");
    return FReply::Handled();
}

void SMassAssetRenamerWindow::OnSubfoldersChanged(ECheckBoxState NewState)
{
    bIncludeSubfolders = (NewState == ECheckBoxState::Checked);
}

void SMassAssetRenamerWindow::OnRegexChanged(ECheckBoxState NewState)
{
    bUseRegex = (NewState == ECheckBoxState::Checked);
}

// ---------- Logging ----------
void SMassAssetRenamerWindow::AddLog(const FString& Message, bool bIsError)
{
    FString LogPrefix = bIsError ? TEXT("[ERROR] ") : TEXT("[INFO] ");
    FString Final = LogPrefix + Message;
    TSharedPtr<STextBlock> LogEntry = SNew(STextBlock)
        .Text(FText::FromString(Final))
        .ColorAndOpacity(bIsError ? FSlateColor(FLinearColor::Red) : FSlateColor(FLinearColor::White))
        .AutoWrapText(true);
    LogPanelBox->AddSlot().AutoHeight().Padding(2)[ LogEntry.ToSharedRef() ];
    if (LogPanel.IsValid())
    {
        LogPanel->ScrollToEnd();
    }
    UE_LOG(LogTemp, Log, TEXT("MassRenamer: %s"), *Final);
}

void SMassAssetRenamerWindow::ClearLog()
{
    LogPanelBox->ClearChildren();
    AddLog("Log cleared.");
}

// ---------- Preview Generation ----------
TArray<FAssetData> SMassAssetRenamerWindow::GetAssetsInFolder(const FString& FolderPath, bool bRecursive) const
{
    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    TArray<FAssetData> Assets;
    AssetRegistryModule.Get().GetAssetsByPath(FName(*FolderPath), Assets, bRecursive);
    // Optionally filter out redirectors, but keep all assets
    return Assets;
}

FString SMassAssetRenamerWindow::ComputeNewName(const FString& OriginalName) const
{
    FString NewName = OriginalName;

    // Apply search/replace
    if (!SearchPattern.IsEmpty())
    {
        if (bUseRegex)
        {
            FRegexPattern Pattern(SearchPattern);
            FRegexMatcher Matcher(Pattern, NewName);
            if (Matcher.FindNext())
            {
                // Simple replace for first match (would need more robust for multiple matches)
                int32 Start = Matcher.GetMatchBeginning();
                int32 End = Matcher.GetMatchEnding();
                NewName = NewName.Left(Start) + ReplacePattern + NewName.Right(NewName.Len() - End);
            }
        }
        else
        {
            NewName = NewName.Replace(*SearchPattern, *ReplacePattern);
        }
    }

    // Apply prefix
    if (!Prefix.IsEmpty())
    {
        NewName = Prefix + NewName;
    }

    // Apply suffix
    if (!Suffix.IsEmpty())
    {
        NewName = NewName + Suffix;
    }

    return NewName;
}

void SMassAssetRenamerWindow::GeneratePreview()
{
    PreviewItems.Empty();
    if (SourceFolder.IsEmpty())
    {
        AddLog("Please select a source folder.", true);
        PreviewListView->RequestListRefresh();
        return;
    }

    TArray<FAssetData> Assets = GetAssetsInFolder(SourceFolder, bIncludeSubfolders);
    if (Assets.Num() == 0)
    {
        AddLog("No assets found in the selected folder.", true);
        PreviewListView->RequestListRefresh();
        return;
    }

    // Update rule strings from UI
    Prefix = PrefixTextBox->GetText().ToString();
    Suffix = SuffixTextBox->GetText().ToString();
    SearchPattern = SearchTextBox->GetText().ToString();
    ReplacePattern = ReplaceTextBox->GetText().ToString();

    for (const FAssetData& Asset : Assets)
    {
        FString OldName = Asset.AssetName.ToString();
        FString NewName = ComputeNewName(OldName);

        bool bValid = (OldName != NewName) && !NewName.IsEmpty();
        if (!bValid && OldName == NewName)
        {
            // No change, still include but mark invalid
        }

        TSharedPtr<FRenamePreviewItem> Item = MakeShared<FRenamePreviewItem>();
        Item->Asset = Asset;
        Item->NewName = NewName;
        Item->bValid = bValid;
        PreviewItems.Add(Item);
    }

    AddLog(FString::Printf(TEXT("Preview generated: %d assets."), PreviewItems.Num()));
    PreviewListView->RequestListRefresh();
}

FReply SMassAssetRenamerWindow::OnRefreshPreview()
{
    GeneratePreview();
    return FReply::Handled();
}

// ---------- Apply Rename ----------
FReply SMassAssetRenamerWindow::OnApplyRename()
{
    if (PreviewItems.Num() == 0)
    {
        AddLog("No preview available. Click 'Refresh Preview' first.", true);
        return FReply::Handled();
    }

    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    TArray<UObject*> ObjectsToRename;
    TArray<FString> NewNames;

    for (const TSharedPtr<FRenamePreviewItem>& Item : PreviewItems)
    {
        if (!Item->bValid) continue;
        UObject* Asset = Item->Asset.GetAsset();
        if (!Asset) continue;
        ObjectsToRename.Add(Asset);
        NewNames.Add(Item->NewName);
    }

    if (ObjectsToRename.Num() == 0)
    {
        AddLog("No valid assets to rename (no changes or invalid names).", true);
        return FReply::Handled();
    }

    // Perform rename
    TArray<FAssetRenameData> RenameData;
    for (int32 i = 0; i < ObjectsToRename.Num(); ++i)
    {
        UObject* Obj = ObjectsToRename[i];
        FString NewName = NewNames[i];
        FString NewPath = FPaths::GetPath(Obj->GetPathName());
        RenameData.Add(FAssetRenameData(Obj, NewPath, NewName));
    }

    bool bSuccess = AssetTools.RenameAssets(RenameData);
    if (bSuccess)
    {
        AddLog(FString::Printf(TEXT("Successfully renamed %d assets."), ObjectsToRename.Num()));
        GeneratePreview(); // Refresh preview after rename
    }
    else
    {
        AddLog("Rename operation failed. Check logs for details.", true);
    }

    return FReply::Handled();
}

// ---------- List View Row Generation ----------
TSharedRef<ITableRow> SMassAssetRenamerWindow::OnGenerateRowForPreview(TSharedPtr<FRenamePreviewItem> Item, const TSharedRef<STableViewBase>& OwnerTable)
{
    FString OldName = Item->Asset.AssetName.ToString();
    FString NewName = Item->NewName;
    FString Status = Item->bValid ? TEXT("OK") : (OldName == NewName ? TEXT("No change") : TEXT("Invalid name"));

    return SNew(STableRow<TSharedPtr<FRenamePreviewItem>>, OwnerTable)
        [
            SNew(SHorizontalBox)
            + SHorizontalBox::Slot().FillWidth(0.4f).Padding(2)[ SNew(STextBlock).Text(FText::FromString(OldName)) ]
            + SHorizontalBox::Slot().FillWidth(0.4f).Padding(2)[ SNew(STextBlock).Text(FText::FromString(NewName)) ]
            + SHorizontalBox::Slot().FillWidth(0.2f).Padding(2)[ SNew(STextBlock).Text(FText::FromString(Status)).ColorAndOpacity(Item->bValid ? FSlateColor(FLinearColor::Green) : FSlateColor(FLinearColor::Yellow)) ]
        ];
}

#undef LOCTEXT_NAMESPACE