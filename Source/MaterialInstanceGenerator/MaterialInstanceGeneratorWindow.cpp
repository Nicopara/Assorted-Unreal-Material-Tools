#include "MaterialInstanceGeneratorWindow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SScrollBox.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "AssetRegistryModule.h"
#include "IAssetTools.h"
#include "AssetToolsModule.h"
#include "EditorDirectories.h"
#include "Factories/MaterialInstanceConstantFactoryNew.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"

#define LOCTEXT_NAMESPACE "MaterialGenerator"

void SMaterialInstanceGeneratorWindow::Construct(const FArguments& InArgs)
{
    ChildSlot
    [
        SNew(SScrollBox)
        + SScrollBox::Slot()
        [
            SNew(SVerticalBox)

            // Master material row
            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [
                    SAssignNew(MasterMaterialTextBox, SEditableTextBox)
                    .HintText(LOCTEXT("MasterMatHint", "/Game/Path/To/MasterMaterial.MasterMaterial"))
                    .OnTextChanged_Lambda([this](const FText& InText) { MasterMaterialPath = InText.ToString(); })
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton).Text(LOCTEXT("BrowseMaster", "Browse")).OnClicked(this, &SMaterialInstanceGeneratorWindow::OnMasterMaterialBrowse)
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton).Text(LOCTEXT("ScanParams", "Scan Parameters")).OnClicked(this, &SMaterialInstanceGeneratorWindow::OnRefreshParameters)
                ]
            ]

            // Output folder row
            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SNew(SHorizontalBox)
                + SHorizontalBox::Slot().FillWidth(1.0f)
                [
                    SAssignNew(OutputFolderTextBox, SEditableTextBox)
                    .HintText(LOCTEXT("OutputHint", "/Game/Path/To/OutputMaterialInstances"))
                    .OnTextChanged_Lambda([this](const FText& InText) { OutputFolderPath = InText.ToString(); })
                ]
                + SHorizontalBox::Slot().AutoWidth()
                [
                    SNew(SButton).Text(LOCTEXT("BrowseOut", "Browse")).OnClicked(this, &SMaterialInstanceGeneratorWindow::OnOutputFolderBrowse)
                ]
            ]

            // Texture parameter mappings header
            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SNew(STextBlock).Text(LOCTEXT("MappingHeader", "Texture Parameters (auto-detected). Assign a folder for each parameter. The first parameter with a folder will drive instance count."))
            ]

            + SVerticalBox::Slot().MaxHeight(200).Padding(5)
            [
                SAssignNew(MappingsScrollBox, SScrollBox)
            ]

            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SNew(SButton).Text(LOCTEXT("AddMapping", "+ Add Custom Parameter")).OnClicked(this, &SMaterialInstanceGeneratorWindow::OnAddMapping)
            ]

            + SVerticalBox::Slot().AutoHeight().Padding(5, 20, 5, 5)
            [
                SNew(SButton)
                .Text(LOCTEXT("GenerateBtn", "Generate Material Instances"))
                .HAlign(HAlign_Center)
                .OnClicked(this, &SMaterialInstanceGeneratorWindow::OnGenerateClicked)
            ]

            + SVerticalBox::Slot().AutoHeight().Padding(5)
            [
                SAssignNew(StatusTextBlock, STextBlock)
                .Text(LOCTEXT("Ready", "Ready."))
            ]
        ]
    ];

    TextureMappings.Empty();
    RefreshMappingsUI();
}

// ---------- Pickers ----------
void SMaterialInstanceGeneratorWindow::PickFolder(FString& OutPath, TSharedPtr<SEditableTextBox> TextBox, const FString& DialogTitle)
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

void SMaterialInstanceGeneratorWindow::PickAsset(FString& OutPath, TSharedPtr<SEditableTextBox> TextBox, const FString& DialogTitle)
{
    IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
    if (DesktopPlatform)
    {
        TArray<FString> OutFiles;
        FString DefaultPath = FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_OPEN);
        if (DesktopPlatform->OpenFileDialog(nullptr, DialogTitle, DefaultPath, TEXT(""), TEXT("Unreal Asset (*.uasset)|*.uasset"), EFileDialogFlags::None, OutFiles))
        {
            if (OutFiles.Num() > 0)
            {
                FString AbsolutePath = OutFiles[0];
                FString ContentDir = FPaths::ProjectContentDir();
                if (AbsolutePath.StartsWith(ContentDir))
                {
                    FString RelativePath = AbsolutePath.Replace(*ContentDir, TEXT("/Game/"));
                    RelativePath = FPaths::ChangeExtension(RelativePath, TEXT(""));
                    OutPath = RelativePath;
                    TextBox->SetText(FText::FromString(OutPath));
                }
                else
                {
                    UpdateStatus("Please select an asset inside the project's Content folder.", true);
                }
            }
        }
    }
}

FReply SMaterialInstanceGeneratorWindow::OnMasterMaterialBrowse()
{
    PickAsset(MasterMaterialPath, MasterMaterialTextBox, "Select Master Material");
    return FReply::Handled();
}

FReply SMaterialInstanceGeneratorWindow::OnOutputFolderBrowse()
{
    PickFolder(OutputFolderPath, OutputFolderTextBox, "Select Output Folder for Material Instances");
    return FReply::Handled();
}

// ---------- Parameter Scanning ----------
void SMaterialInstanceGeneratorWindow::ScanMaterialParameters()
{
    if (MasterMaterialPath.IsEmpty())
    {
        UpdateStatus("Please select a Master Material first.", true);
        return;
    }

    UMaterialInterface* MaterialInterface = LoadObject<UMaterialInterface>(nullptr, *MasterMaterialPath);
    if (!MaterialInterface)
    {
        UpdateStatus("Failed to load Master Material.", true);
        return;
    }

    UMaterial* BaseMaterial = MaterialInterface->GetMaterial();
    if (!BaseMaterial)
    {
        UpdateStatus("Could not retrieve base material.", true);
        return;
    }

    TArray<FMaterialParameterInfo> TexParams;
    TArray<FGuid> Guids;
    BaseMaterial->GetAllTextureParameterInfo(TexParams, Guids);

    if (TexParams.Num() == 0)
    {
        UpdateStatus("No texture parameters found in this material.", true);
        return;
    }

    TextureMappings.Empty();
    for (const FMaterialParameterInfo& Info : TexParams)
    {
        FTextureParameterMapping NewMapping;
        NewMapping.ParameterName = Info.Name.ToString();
        NewMapping.FolderPath = TEXT("");
        TextureMappings.Add(NewMapping);
    }

    RefreshMappingsUI();
    UpdateStatus(FString::Printf(TEXT("Found %d texture parameters. Assign texture folders below. The first parameter with a folder will drive instance count."), TexParams.Num()));
}

FReply SMaterialInstanceGeneratorWindow::OnRefreshParameters()
{
    ScanMaterialParameters();
    return FReply::Handled();
}

// ---------- UI Management ----------
FReply SMaterialInstanceGeneratorWindow::OnAddMapping()
{
    TextureMappings.Add(FTextureParameterMapping{TEXT("CustomParam"), TEXT("")});
    RefreshMappingsUI();
    return FReply::Handled();
}

void SMaterialInstanceGeneratorWindow::RefreshMappingsUI()
{
    MappingsScrollBox->ClearChildren();

    for (int32 i = 0; i < TextureMappings.Num(); ++i)
    {
        FTextureParameterMapping& Mapping = TextureMappings[i];

        TSharedPtr<SEditableTextBox> ParamTextBox;
        TSharedPtr<SEditableTextBox> FolderTextBox;

        auto HorizontalBox = SNew(SHorizontalBox);

        HorizontalBox->AddSlot().FillWidth(0.3f).Padding(2)
        [
            SAssignNew(ParamTextBox, SEditableTextBox)
            .Text(FText::FromString(Mapping.ParameterName))
            .HintText(LOCTEXT("ParamNameHint", "Parameter Name"))
            .OnTextCommitted_Lambda([this, i](const FText& NewText, ETextCommit::Type) { TextureMappings[i].ParameterName = NewText.ToString(); })
        ];

        HorizontalBox->AddSlot().FillWidth(0.6f).Padding(2)
        [
            SAssignNew(FolderTextBox, SEditableTextBox)
            .Text(FText::FromString(Mapping.FolderPath))
            .HintText(LOCTEXT("TexFolderHintMapping", "/Game/Path/To/Textures"))
            .OnTextCommitted_Lambda([this, i](const FText& NewText, ETextCommit::Type) { TextureMappings[i].FolderPath = NewText.ToString(); })
        ];

        HorizontalBox->AddSlot().AutoWidth().Padding(2)
        [
            SNew(SButton).Text(LOCTEXT("Browse", "Browse")).OnClicked_Lambda([this, i, FolderTextBox]() -> FReply
            {
                PickFolder(TextureMappings[i].FolderPath, FolderTextBox, "Select Texture Folder");
                return FReply::Handled();
            })
        ];

        HorizontalBox->AddSlot().AutoWidth().Padding(2)
        [
            SNew(SButton).Text(LOCTEXT("Remove", "X")).OnClicked_Lambda([this, i]() -> FReply
            {
                RemoveMapping(i);
                return FReply::Handled();
            })
        ];

        MappingsScrollBox->AddSlot().Padding(2)
        [
            HorizontalBox
        ];
    }
}

void SMaterialInstanceGeneratorWindow::RemoveMapping(int32 Index)
{
    if (TextureMappings.IsValidIndex(Index))
    {
        TextureMappings.RemoveAt(Index);
        RefreshMappingsUI();
    }
}

// ---------- Generate Material Instances ----------
FReply SMaterialInstanceGeneratorWindow::OnGenerateClicked()
{
    GenerateMaterialInstances();
    return FReply::Handled();
}

void SMaterialInstanceGeneratorWindow::GenerateMaterialInstances()
{
    if (MasterMaterialPath.IsEmpty() || OutputFolderPath.IsEmpty())
    {
        UpdateStatus("Error: Master Material and Output Folder are required.", true);
        return;
    }

    // Find the driver parameter (BaseColor/Albedo etc.) with a non‑empty folder.
    int32 DriverIndex = -1;
    TArray<FString> PreferredDriverNames = { TEXT("BaseColor"), TEXT("Albedo"), TEXT("Diffuse"), TEXT("Base Colour") };
    
    for (int32 i = 0; i < TextureMappings.Num(); ++i)
    {
        if (TextureMappings[i].FolderPath.IsEmpty()) continue;
        FString ParamName = TextureMappings[i].ParameterName;
        for (const FString& Preferred : PreferredDriverNames)
        {
            if (ParamName.Equals(Preferred, ESearchCase::IgnoreCase))
            {
                DriverIndex = i;
                break;
            }
        }
        if (DriverIndex != -1) break;
    }
    
    if (DriverIndex == -1)
    {
        for (int32 i = 0; i < TextureMappings.Num(); ++i)
        {
            if (!TextureMappings[i].FolderPath.IsEmpty())
            {
                DriverIndex = i;
                UpdateStatus(FString::Printf(TEXT("Warning: No standard albedo parameter found. Using '%s' as driver."), *TextureMappings[DriverIndex].ParameterName), true);
                break;
            }
        }
    }
    
    if (DriverIndex == -1)
    {
        UpdateStatus("Error: No texture folder assigned to any parameter. Please assign at least one folder.", true);
        return;
    }

    UMaterialInterface* MasterMaterial = LoadObject<UMaterialInterface>(nullptr, *MasterMaterialPath);
    if (!MasterMaterial)
    {
        UpdateStatus("Error: Could not load Master Material.", true);
        return;
    }

    FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
    IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
    UMaterialInstanceConstantFactoryNew* Factory = NewObject<UMaterialInstanceConstantFactoryNew>();
    Factory->InitialParent = MasterMaterial;

    // Gather all textures from the driver folder.
    TArray<FAssetData> DriverTextures;
    AssetRegistryModule.Get().GetAssetsByPath(FName(*TextureMappings[DriverIndex].FolderPath), DriverTextures, true);

    int32 SuccessCount = 0;

    for (const FAssetData& DriverAsset : DriverTextures)
    {
        UTexture2D* DriverTex = Cast<UTexture2D>(DriverAsset.GetAsset());
        if (!DriverTex) continue;

        FString BaseName = DriverTex->GetName();
        FString CleanBase = BaseName;
        CleanBase = CleanBase.Replace(TEXT("_D"), TEXT("")).Replace(TEXT("_BaseColor"), TEXT("")).Replace(TEXT("_Diffuse"), TEXT(""))
                              .Replace(TEXT("_Albedo"), TEXT("")).Replace(TEXT("_Color"), TEXT(""));

        FString InstanceName = FString::Printf(TEXT("MI_%s"), *BaseName);
        UObject* NewAsset = AssetTools.CreateAsset(InstanceName, OutputFolderPath, UMaterialInstanceConstant::StaticClass(), Factory);
        UMaterialInstanceConstant* NewMI = Cast<UMaterialInstanceConstant>(NewAsset);
        if (!NewMI)
        {
            UpdateStatus(FString::Printf(TEXT("Failed to create MI for %s"), *BaseName), true);
            continue;
        }

        NewMI->SetParentEditorOnly(MasterMaterial);

        // Set texture parameters.
        for (int32 i = 0; i < TextureMappings.Num(); ++i)
        {
            const FTextureParameterMapping& Mapping = TextureMappings[i];
            if (Mapping.FolderPath.IsEmpty()) continue;

            if (i == DriverIndex)
            {
                NewMI->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(*Mapping.ParameterName), DriverTex);
                continue;
            }

            // For non‑driver parameters, try to find a texture that contains the clean base name.
            TArray<FAssetData> TexAssets;
            AssetRegistryModule.Get().GetAssetsByPath(FName(*Mapping.FolderPath), TexAssets, true);
            for (const FAssetData& TexData : TexAssets)
            {
                UTexture2D* Tex = Cast<UTexture2D>(TexData.GetAsset());
                if (Tex && Tex->GetName().Contains(CleanBase))
                {
                    NewMI->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(*Mapping.ParameterName), Tex);
                    break;
                }
            }
        }

        NewMI->PostEditChange();
        NewMI->MarkPackageDirty();
        SuccessCount++;
    }

    UpdateStatus(FString::Printf(TEXT("Generated %d material instances using driver parameter '%s'."), SuccessCount, *TextureMappings[DriverIndex].ParameterName));
}

void SMaterialInstanceGeneratorWindow::UpdateStatus(const FString& Message, bool bIsError)
{
    StatusTextBlock->SetText(FText::FromString(Message));
}

#undef LOCTEXT_NAMESPACE