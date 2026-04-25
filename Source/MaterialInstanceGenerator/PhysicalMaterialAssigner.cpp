#include "PhysicalMaterialAssigner.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "AssetRegistryModule.h"
#include "Materials/MaterialInstanceConstant.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "EditorDirectories.h"

#define LOCTEXT_NAMESPACE "PhysicalMaterialAssigner"

// -----------------------------------------------------------------------------
// SPhysicalMaterialAssignerWindow Implementation
// -----------------------------------------------------------------------------

void SPhysicalMaterialAssignerWindow::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SScrollBox)
        + SScrollBox::Slot()
        [
            SNew(SVerticalBox)

            // Material Instances folder
            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [
                    SAssignNew(MIFolderTextBox, SEditableTextBox)
                    .HintText(LOCTEXT("MIFolderHint", "/Game/Path/To/MaterialInstances"))
                    .OnTextChanged_Lambda([this](const FText& InText) { MaterialInstancesFolder = InText.ToString(); })
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton).Text(LOCTEXT("BrowseMI", "Browse")).OnClicked(this, &SPhysicalMaterialAssignerWindow::OnMIFolderBrowse)
                ]
            ]

            // Physical Materials folder
            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [
                    SAssignNew(PhysMatFolderTextBox, SEditableTextBox)
                    .HintText(LOCTEXT("PhysMatHint", "/Game/Path/To/PhysicalMaterials"))
                    .OnTextChanged_Lambda([this](const FText& InText) { PhysicalMaterialsFolder = InText.ToString(); })
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton).Text(LOCTEXT("BrowsePhys", "Browse")).OnClicked(this, &SPhysicalMaterialAssignerWindow::OnPhysMatFolderBrowse)
                ]
            ]

            + SVerticalBox::Slot().AutoHeight().Padding(5, 20, 5, 5)
            [
                SNew(SButton)
                .Text(LOCTEXT("AssignBtn", "Assign Physical Materials"))
                .HAlign(HAlign_Center)
                .OnClicked(this, &SPhysicalMaterialAssignerWindow::OnAssignClicked)
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
void SPhysicalMaterialAssignerWindow::PickFolder(FString& OutPath, TSharedPtr<SEditableTextBox> TextBox, const FString& DialogTitle)
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

FReply SPhysicalMaterialAssignerWindow::OnMIFolderBrowse()
{
    PickFolder(MaterialInstancesFolder, MIFolderTextBox, "Select Material Instances Folder");
    return FReply::Handled();
}

FReply SPhysicalMaterialAssignerWindow::OnPhysMatFolderBrowse()
{
    PickFolder(PhysicalMaterialsFolder, PhysMatFolderTextBox, "Select Physical Materials Folder");
    return FReply::Handled();
}

FReply SPhysicalMaterialAssignerWindow::OnAssignClicked()
{
    AssignPhysicalMaterials();
    return FReply::Handled();
}

// ---------- Core Assignment Logic ----------
FString SPhysicalMaterialAssignerWindow::FindBestPhysicalMaterialAsset(const FString& MaterialInstanceName, const TArray<FAssetData>& PhysicalAssets) const
{
    // Clean the MI name (remove "MI_" prefix if present)
    FString CleanName = MaterialInstanceName;
    if (CleanName.StartsWith(TEXT("MI_")))
        CleanName = CleanName.RightChop(3);

    // 1. Collect candidate keywords
    TArray<FString> Keywords;

    // Hardcoded common material list (includes all requested additions)
    static const TArray<FString> CommonMaterials = {
        TEXT("wood"), TEXT("metal"), TEXT("stone"), TEXT("plastic"), TEXT("glass"),
        TEXT("fabric"), TEXT("rubber"), TEXT("concrete"), TEXT("dirt"), TEXT("sand"),
        TEXT("ice"), TEXT("water"), TEXT("grass"), TEXT("leather"), TEXT("bone"),
        TEXT("flesh"), TEXT("paper"), TEXT("cloth"), TEXT("gravel"), TEXT("mud"),
        TEXT("snow"), TEXT("brick"), TEXT("ceramic"), TEXT("carpet"), TEXT("tile"),
        TEXT("blood"), TEXT("hell"), TEXT("meat"), TEXT("alien"),
        TEXT("rock"), TEXT("marble"), TEXT("asphalt"), TEXT("clay"), TEXT("rust")
    };

    FString LowerClean = CleanName.ToLower();
    for (const FString& Mat : CommonMaterials)
    {
        if (LowerClean.Contains(Mat))
        {
            Keywords.Add(Mat);
        }
    }

    // 2. If no keyword was found, tokenize the name (split by underscore/space/camelCase)
    if (Keywords.Num() == 0)
    {
        TArray<FString> Tokens;
        // Replace separators with spaces
        FString Temp = LowerClean;
        Temp.ReplaceInline(TEXT("_"), TEXT(" "));
        Temp.ParseIntoArray(Tokens, TEXT(" "), true);

        // Handle camelCase (e.g., "BrickWall" -> ["brick","wall"])
        if (CleanName.Len() > 0 && !CleanName.Contains(TEXT("_")) && !CleanName.Contains(TEXT(" ")))
        {
            FString CurrentToken;
            for (int32 i = 0; i < CleanName.Len(); ++i)
            {
                TCHAR Ch = CleanName[i];
                if (FChar::IsUpper(Ch) && CurrentToken.Len() > 0)
                {
                    Tokens.Add(CurrentToken.ToLower());
                    CurrentToken.Empty();
                }
                CurrentToken.AppendChar(Ch);
            }
            if (CurrentToken.Len() > 0)
            {
                Tokens.Add(CurrentToken.ToLower());
            }
        }

        // Add tokens that are long enough
        for (const FString& Token : Tokens)
        {
            if (Token.Len() >= 2)
                Keywords.Add(Token);
        }
    }

    // 3. Score physical material assets against all keywords
    float BestScore = -1.0f;
    FString BestAssetPath;

    for (const FAssetData& Asset : PhysicalAssets)
    {
        FString AssetName = Asset.AssetName.ToString().ToLower();

        for (const FString& Keyword : Keywords)
        {
            if (AssetName.Contains(Keyword))
            {
                float Score = (AssetName == Keyword) ? 2.0f : 1.0f;
                if (AssetName.StartsWith(Keyword))
                    Score += 0.5f;
                // Slight penalty for very short keywords to avoid false positives
                if (Keyword.Len() < 3)
                    Score -= 0.2f;

                if (Score > BestScore)
                {
                    BestScore = Score;
                    BestAssetPath = Asset.ObjectPath.ToString();
                }
            }
        }
    }

    return BestAssetPath;
}

void SPhysicalMaterialAssignerWindow::AssignPhysicalMaterials()
{
    if (MaterialInstancesFolder.IsEmpty() || PhysicalMaterialsFolder.IsEmpty())
    {
        UpdateStatus("Error: Material Instances folder and Physical Materials folder are required.", true);
        return;
    }

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");

    TArray<FAssetData> PhysicalAssets;
    AssetRegistryModule.Get().GetAssetsByPath(FName(*PhysicalMaterialsFolder), PhysicalAssets, true);
    PhysicalAssets.RemoveAll([](const FAssetData& Asset) {
        return Asset.AssetClass != UPhysicalMaterial::StaticClass()->GetFName();
    });

    if (PhysicalAssets.Num() == 0)
    {
        UpdateStatus("No physical material assets found in the specified folder.", true);
        return;
    }

    TArray<FAssetData> MIList;
    AssetRegistryModule.Get().GetAssetsByPath(FName(*MaterialInstancesFolder), MIList, true);
    MIList.RemoveAll([](const FAssetData& Asset) {
        return Asset.AssetClass != UMaterialInstanceConstant::StaticClass()->GetFName();
    });

    if (MIList.Num() == 0)
    {
        UpdateStatus("No material instances found in the specified folder.", true);
        return;
    }

    int32 AssignedCount = 0;
    int32 SkippedCount = 0;

    for (const FAssetData& MIData : MIList)
    {
        UMaterialInstanceConstant* MI = Cast<UMaterialInstanceConstant>(MIData.GetAsset());
        if (!MI) continue;

        FString BestPhysMatPath = FindBestPhysicalMaterialAsset(MI->GetName(), PhysicalAssets);
        if (BestPhysMatPath.IsEmpty())
        {
            UpdateStatus(FString::Printf(TEXT("Skipping %s: no matching physical material found."), *MI->GetName()), true);
            SkippedCount++;
            continue;
        }

        UPhysicalMaterial* PhysMat = LoadObject<UPhysicalMaterial>(nullptr, *BestPhysMatPath);
        if (!PhysMat)
        {
            UpdateStatus(FString::Printf(TEXT("Failed to load physical material %s"), *BestPhysMatPath), true);
            SkippedCount++;
            continue;
        }

        MI->PhysMaterial = PhysMat;
        MI->MarkPackageDirty();
        AssignedCount++;
    }

    UpdateStatus(FString::Printf(TEXT("Assigned physical materials to %d material instances. Skipped %d."), AssignedCount, SkippedCount));
}

void SPhysicalMaterialAssignerWindow::UpdateStatus(const FString& Message, bool bIsError)
{
    StatusTextBlock->SetText(FText::FromString(Message));
    if (bIsError)
    {
        UE_LOG(LogTemp, Warning, TEXT("PhysicalMaterialAssigner: %s"), *Message);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("PhysicalMaterialAssigner: %s"), *Message);
    }
}

#undef LOCTEXT_NAMESPACE