#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class STextBlock;

class SPhysicalMaterialAssignerWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPhysicalMaterialAssignerWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FString MaterialInstancesFolder;
	FString PhysicalMaterialsFolder;

	TSharedPtr<SEditableTextBox> MIFolderTextBox;
	TSharedPtr<SEditableTextBox> PhysMatFolderTextBox;
	TSharedPtr<STextBlock> StatusTextBlock;

	FReply OnMIFolderBrowse();
	FReply OnPhysMatFolderBrowse();
	FReply OnAssignClicked();

	void PickFolder(FString& OutPath, TSharedPtr<SEditableTextBox> TextBox, const FString& DialogTitle);
	void UpdateStatus(const FString& Message, bool bIsError = false);
	void AssignPhysicalMaterials();
	FString FindBestPhysicalMaterialAsset(const FString& MaterialInstanceName, const TArray<FAssetData>& PhysicalAssets) const;
};