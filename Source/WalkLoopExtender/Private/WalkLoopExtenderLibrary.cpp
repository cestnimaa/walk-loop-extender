#include "WalkLoopExtenderLibrary.h"

#include "Animation/AnimData/IAnimationDataController.h"
#include "Animation/AnimData/IAnimationDataModel.h"
#include "Animation/AnimSequence.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetToolsModule.h"
#include "Misc/PackageName.h"
#include "ScopedTransaction.h"
#include "WalkLoopExtenderSettings.h"

#define LOCTEXT_NAMESPACE "WalkLoopExtenderLibrary"

namespace WalkLoopExtender
{
    struct FSourceTrack
    {
        FName BoneName;
        TArray<FTransform> Transforms;
    };

    static FTransform SampleTrack(const TArray<FTransform>& SourceTransforms, double SourceFrame)
    {
        if (SourceTransforms.Num() == 0)
        {
            return FTransform::Identity;
        }

        if (SourceTransforms.Num() == 1)
        {
            return SourceTransforms[0];
        }

        const double ClampedFrame = FMath::Clamp(SourceFrame, 0.0, static_cast<double>(SourceTransforms.Num() - 1));
        const int32 LowerIndex = FMath::Clamp(FMath::FloorToInt(ClampedFrame), 0, SourceTransforms.Num() - 1);
        const int32 UpperIndex = FMath::Min(LowerIndex + 1, SourceTransforms.Num() - 1);
        const double Alpha = ClampedFrame - static_cast<double>(LowerIndex);

        const FTransform& Lower = SourceTransforms[LowerIndex];
        const FTransform& Upper = SourceTransforms[UpperIndex];

        const FVector Location = FMath::Lerp(Lower.GetLocation(), Upper.GetLocation(), Alpha);
        const FQuat Rotation = FQuat::Slerp(Lower.GetRotation(), Upper.GetRotation(), Alpha).GetNormalized();
        const FVector Scale = FMath::Lerp(Lower.GetScale3D(), Upper.GetScale3D(), Alpha);

        return FTransform(Rotation, Location, Scale);
    }

    static UAnimSequence* DuplicateAnimationAsset(UAnimSequence* SourceAnimation, const FString& OutputSuffix, FString& OutMessage)
    {
        if (!SourceAnimation)
        {
            OutMessage = TEXT("No source animation was provided.");
            return nullptr;
        }

        const FString SourcePackageName = SourceAnimation->GetOutermost()->GetName();
        if (!FPackageName::IsValidLongPackageName(SourcePackageName))
        {
            OutMessage = TEXT("The source animation must be saved in the Content Browser before it can be duplicated.");
            return nullptr;
        }

        const FString PackagePath = FPackageName::GetLongPackagePath(SourcePackageName);
        const FString DesiredAssetName = SourceAnimation->GetName() + (OutputSuffix.IsEmpty() ? TEXT("_Loop600") : OutputSuffix);
        const FString DesiredPackageName = PackagePath / DesiredAssetName;

        FAssetToolsModule& AssetToolsModule = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools");
        FString UniquePackageName;
        FString UniqueAssetName;
        AssetToolsModule.Get().CreateUniqueAssetName(DesiredPackageName, TEXT(""), UniquePackageName, UniqueAssetName);

        UObject* DuplicatedObject = AssetToolsModule.Get().DuplicateAsset(UniqueAssetName, PackagePath, SourceAnimation);
        UAnimSequence* DuplicatedAnimation = Cast<UAnimSequence>(DuplicatedObject);
        if (!DuplicatedAnimation)
        {
            OutMessage = TEXT("Unreal could not duplicate the animation asset.");
            return nullptr;
        }

        FAssetRegistryModule::AssetCreated(DuplicatedAnimation);
        DuplicatedAnimation->MarkPackageDirty();
        return DuplicatedAnimation;
    }

    static bool BuildSourceTracks(
        const IAnimationDataModel* SourceModel,
        TArray<FSourceTrack>& OutTracks,
        FString& OutMessage)
    {
        if (!SourceModel)
        {
            OutMessage = TEXT("The animation has no valid animation data model.");
            return false;
        }

        TArray<FName> TrackNames;
        SourceModel->GetBoneTrackNames(TrackNames);

        if (TrackNames.Num() == 0)
        {
            OutMessage = TEXT("The animation has no bone animation tracks to extend.");
            return false;
        }

        OutTracks.Reset(TrackNames.Num());

        for (const FName& TrackName : TrackNames)
        {
            FSourceTrack SourceTrack;
            SourceTrack.BoneName = TrackName;
            SourceModel->GetBoneTrackTransforms(TrackName, SourceTrack.Transforms);

            if (SourceTrack.Transforms.Num() > 0)
            {
                OutTracks.Add(MoveTemp(SourceTrack));
            }
        }

        if (OutTracks.Num() == 0)
        {
            OutMessage = TEXT("The animation's bone tracks did not contain transform keys.");
            return false;
        }

        return true;
    }

    static const FSourceTrack* FindTrack(const TArray<FSourceTrack>& Tracks, FName BoneName)
    {
        return Tracks.FindByPredicate([BoneName](const FSourceTrack& Track)
        {
            return Track.BoneName == BoneName;
        });
    }
}

FWalkLoopExtendOptions::FWalkLoopExtendOptions()
    : TargetFrameCount(600)
    , RootBoneName(NAME_None)
    , bFlattenAccumulatedZ(true)
    , bCreateDuplicate(true)
    , OutputSuffix(TEXT("_Loop600"))
{
}

UWalkLoopExtenderSettings::UWalkLoopExtenderSettings()
    : TargetFrameCount(600)
    , RootBoneName(NAME_None)
    , bFlattenAccumulatedZ(true)
    , OutputSuffix(TEXT("_Loop600"))
{
}

FName UWalkLoopExtenderSettings::GetCategoryName() const
{
    return TEXT("Plugins");
}

FText UWalkLoopExtenderSettings::GetSectionText() const
{
    return LOCTEXT("WalkLoopExtenderSettingsSection", "Walk Loop Extender");
}

FWalkLoopExtendOptions UWalkLoopExtenderLibrary::MakeOptionsFromSettings()
{
    const UWalkLoopExtenderSettings* Settings = GetDefault<UWalkLoopExtenderSettings>();

    FWalkLoopExtendOptions Options;
    Options.TargetFrameCount = Settings ? Settings->TargetFrameCount : 600;
    Options.RootBoneName = Settings ? Settings->RootBoneName : NAME_None;
    Options.bFlattenAccumulatedZ = Settings ? Settings->bFlattenAccumulatedZ : true;
    Options.OutputSuffix = Settings ? Settings->OutputSuffix : TEXT("_Loop600");
    Options.bCreateDuplicate = true;
    return Options;
}

bool UWalkLoopExtenderLibrary::CreateExtendedWalkLoop(
    UAnimSequence* SourceAnimation,
    FWalkLoopExtendOptions Options,
    UAnimSequence*& OutAnimation,
    FString& OutMessage)
{
    OutAnimation = nullptr;
    OutMessage.Reset();

    if (!SourceAnimation)
    {
        OutMessage = TEXT("Select a valid AnimSequence.");
        return false;
    }

    if (Options.TargetFrameCount < 2)
    {
        OutMessage = TEXT("Target frame count must be at least 2.");
        return false;
    }

    const IAnimationDataModel* SourceModel = SourceAnimation->GetDataModel();
    if (!SourceModel)
    {
        OutMessage = FString::Printf(TEXT("%s has no valid animation data model."), *SourceAnimation->GetName());
        return false;
    }

    TArray<WalkLoopExtender::FSourceTrack> SourceTracks;
    if (!WalkLoopExtender::BuildSourceTracks(SourceModel, SourceTracks, OutMessage))
    {
        return false;
    }

    FName RootTrackName = Options.RootBoneName;
    if (RootTrackName.IsNone() || !WalkLoopExtender::FindTrack(SourceTracks, RootTrackName))
    {
        RootTrackName = SourceTracks[0].BoneName;
    }

    const WalkLoopExtender::FSourceTrack* RootTrack = WalkLoopExtender::FindTrack(SourceTracks, RootTrackName);
    if (!RootTrack || RootTrack->Transforms.Num() < 2)
    {
        OutMessage = FString::Printf(TEXT("%s does not have enough root-track keys to extend."), *SourceAnimation->GetName());
        return false;
    }

    const int32 SourceFrameCount = SourceModel->GetNumberOfFrames();
    const int32 SourceLoopFrameCount = FMath::Clamp(SourceFrameCount, 1, RootTrack->Transforms.Num() - 1);
    if (SourceLoopFrameCount < 1)
    {
        OutMessage = FString::Printf(TEXT("%s does not contain a usable loop range."), *SourceAnimation->GetName());
        return false;
    }

    FVector LoopDelta = RootTrack->Transforms[SourceLoopFrameCount].GetLocation() - RootTrack->Transforms[0].GetLocation();
    if (Options.bFlattenAccumulatedZ)
    {
        LoopDelta.Z = 0.0;
    }

    UAnimSequence* TargetAnimation = SourceAnimation;
    if (Options.bCreateDuplicate)
    {
        TargetAnimation = WalkLoopExtender::DuplicateAnimationAsset(SourceAnimation, Options.OutputSuffix, OutMessage);
        if (!TargetAnimation)
        {
            return false;
        }
    }

    const FScopedTransaction Transaction(LOCTEXT("CreateExtendedWalkLoopTransaction", "Create Extended Walk Loop"));
    TargetAnimation->Modify();

    IAnimationDataController& Controller = TargetAnimation->GetController();
    Controller.OpenBracket(LOCTEXT("CreateExtendedWalkLoopBracket", "Create Extended Walk Loop"), true);

    const FFrameRate FrameRate = SourceModel->GetFrameRate();
    Controller.SetFrameRate(FrameRate, false);
    Controller.SetNumberOfFrames(FFrameNumber(Options.TargetFrameCount), false);

    const IAnimationDataModel* TargetModel = TargetAnimation->GetDataModel();
    const int32 TargetKeyCount = TargetModel ? TargetModel->GetNumberOfKeys() : Options.TargetFrameCount + 1;
    if (TargetKeyCount < 2)
    {
        Controller.CloseBracket(true);
        OutMessage = FString::Printf(TEXT("%s could not create enough target keys."), *TargetAnimation->GetName());
        return false;
    }

    bool bAllTracksSucceeded = true;

    for (const WalkLoopExtender::FSourceTrack& SourceTrack : SourceTracks)
    {
        TArray<FVector> PositionKeys;
        TArray<FQuat> RotationKeys;
        TArray<FVector> ScaleKeys;
        PositionKeys.Reserve(TargetKeyCount);
        RotationKeys.Reserve(TargetKeyCount);
        ScaleKeys.Reserve(TargetKeyCount);

        const bool bIsRootTrack = SourceTrack.BoneName == RootTrackName;

        for (int32 TargetKeyIndex = 0; TargetKeyIndex < TargetKeyCount; ++TargetKeyIndex)
        {
            const double ContinuousSourceFrame = static_cast<double>(TargetKeyIndex);
            const int32 CompletedLoops = FMath::FloorToInt(ContinuousSourceFrame / static_cast<double>(SourceLoopFrameCount));
            const double LocalSourceFrame = FMath::Fmod(ContinuousSourceFrame, static_cast<double>(SourceLoopFrameCount));

            FTransform Sample = WalkLoopExtender::SampleTrack(SourceTrack.Transforms, LocalSourceFrame);
            if (bIsRootTrack)
            {
                Sample.AddToTranslation(LoopDelta * static_cast<double>(CompletedLoops));
            }

            PositionKeys.Add(Sample.GetLocation());
            RotationKeys.Add(Sample.GetRotation().GetNormalized());
            ScaleKeys.Add(Sample.GetScale3D());
        }

        bAllTracksSucceeded &= Controller.SetBoneTrackKeys(SourceTrack.BoneName, PositionKeys, RotationKeys, ScaleKeys, false);
    }

    Controller.CloseBracket(true);

    TargetAnimation->PostEditChange();
    TargetAnimation->MarkPackageDirty();

    OutAnimation = TargetAnimation;

    const double FramesPerSecond = FrameRate.AsDecimal();
    const double DurationSeconds = FramesPerSecond > 0.0 ? static_cast<double>(Options.TargetFrameCount) / FramesPerSecond : 0.0;
    OutMessage = FString::Printf(
        TEXT("%s -> %s, %d frames at %.2f fps, root track '%s', per-loop travel %s."),
        *SourceAnimation->GetName(),
        *TargetAnimation->GetName(),
        Options.TargetFrameCount,
        FramesPerSecond,
        *RootTrackName.ToString(),
        *LoopDelta.ToString());

    return bAllTracksSucceeded;
}

#undef LOCTEXT_NAMESPACE
