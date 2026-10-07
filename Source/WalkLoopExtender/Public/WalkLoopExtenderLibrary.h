#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WalkLoopExtenderLibrary.generated.h"

class UAnimSequence;

USTRUCT(BlueprintType)
struct WALKLOOPEXTENDER_API FWalkLoopExtendOptions
{
    GENERATED_BODY()

    FWalkLoopExtendOptions();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk Loop", meta = (ClampMin = "2", UIMin = "2"))
    int32 TargetFrameCount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk Loop")
    FName RootBoneName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Walk Loop")
    bool bFlattenAccumulatedZ;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
    bool bCreateDuplicate;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Output")
    FString OutputSuffix;
};

UCLASS()
class WALKLOOPEXTENDER_API UWalkLoopExtenderLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Animation|Walk Loop Extender", meta = (DisplayName = "Create Extended Walk Loop"))
    static bool CreateExtendedWalkLoop(UAnimSequence* SourceAnimation, FWalkLoopExtendOptions Options, UAnimSequence*& OutAnimation, FString& OutMessage);

    static FWalkLoopExtendOptions MakeOptionsFromSettings();
};
