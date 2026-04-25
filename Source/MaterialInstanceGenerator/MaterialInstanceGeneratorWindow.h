#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

struct FTextureParameterMapping
{
	FString ParameterName;
	FString FolderPath;
};

class SEditableTextBox;
class SScrollBox;
class STextBlock;

class SMaterialInstanceGeneratorWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMaterialInstanceGeneratorWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FString MasterMaterialPath;
	FString OutputFolderPath;
	TArray<FTextureParameterMapping> TextureMappings;

	TSharedPtr<SEditableTextBox> MasterMaterialTextBox;
	TSharedPtr<SEditableTextBox> OutputFolderTextBox;
	TSharedPtr<SScrollBox> MappingsScrollBox;
	TSharedPtr<STextBlock> StatusTextBlock;

	FReply OnMasterMaterialBrowse();
	FReply OnOutputFolderBrowse();
	FReply OnRefreshParameters();
	FReply OnAddMapping();
	FReply OnGenerateClicked();

	void PickFolder(FString& OutPath, TSharedPtr<SEditableTextBox> TextBox, const FString& DialogTitle);
	void PickAsset(FString& OutPath, TSharedPtr<SEditableTextBox> TextBox, const FString& DialogTitle);
	void UpdateStatus(const FString& Message, bool bIsError = false);

	void GenerateMaterialInstances();
	void RefreshMappingsUI();
	void RemoveMapping(int32 Index);
	void ScanMaterialParameters();
};