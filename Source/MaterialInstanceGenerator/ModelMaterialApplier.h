#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

class SEditableTextBox;
class SScrollBox;
class STextBlock;

class SModelMaterialApplierWindow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SModelMaterialApplierWindow) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

private:
	FString SourceModelsFolder;
	FString TargetModelsFolder;
	FString MaterialInstancesFolder;

	TSharedPtr<SEditableTextBox> SourceModelsTextBox;
	TSharedPtr<SEditableTextBox> TargetModelsTextBox;
	TSharedPtr<SEditableTextBox> MaterialInstancesTextBox;
	TSharedPtr<STextBlock> StatusTextBlock;

	FReply OnSourceModelsBrowse();
	FReply OnTargetModelsBrowse();
	FReply OnMaterialInstancesBrowse();
	FReply OnApplyClicked();

	void PickFolder(FString& OutPath, TSharedPtr<SEditableTextBox> TextBox, const FString& DialogTitle);
	void UpdateStatus(const FString& Message, bool bIsError = false);

	void ApplyMaterialInstancesToModels();
};