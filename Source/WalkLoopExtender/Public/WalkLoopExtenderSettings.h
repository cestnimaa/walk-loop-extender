#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "WalkLoopExtenderSettings.generated.h"

UCLASS(Config = EditorPerProjectUserSettings, DefaultConfig, meta = (DisplayName = "Walk Loop Extender"))
class WALKLOOPEXTENDER_API UWalkLoopExtenderSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    UWalkLoopExtenderSettings();

    virtual FName GetCategoryName() const override;
    virtual FText GetSectionText() const override;

    UPROPERTY(Config, EditAnywhere, Category = "Walk Loop", meta = (ClampMin = "2", UIMin = "2"))
    int32 TargetFrameCount;

    UPROPERTY(Config, EditAnywhere, Category = "Walk Loop")
    FName RootBoneName;

    UPROPERTY(Config, EditAnywhere, Category = "Walk Loop")
    bool bFlattenAccumulatedZ;

    UPROPERTY(Config, EditAnywhere, Category = "Output")
    FString OutputSuffix;
};
