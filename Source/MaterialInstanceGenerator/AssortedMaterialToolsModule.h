#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FAssortedMaterialToolsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterMenus();

	TSharedRef<class SDockTab> OnSpawnGeneratorTab(const class FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<class SDockTab> OnSpawnAssignerTab(const class FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<class SDockTab> OnSpawnApplierTab(const class FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<class SDockTab> OnSpawnRenamerTab(const class FSpawnTabArgs& SpawnTabArgs);
	TSharedRef<class SDockTab> OnSpawnConsolidatorTab(const class FSpawnTabArgs& SpawnTabArgs);
	
	void OpenConsolidatorWindow();
	void OpenRenamerWindow();
	void OpenGeneratorWindow();
	void OpenAssignerWindow();
	void OpenApplierWindow();
};