#include "WalkLoopExtenderLibrary.h"

#include "Animation/AnimSequence.h"
#include "ContentBrowserModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "IContentBrowserSingleton.h"
#include "Modules/ModuleManager.h"
#include "ToolMenus.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "FWalkLoopExtenderModule"

class FWalkLoopExtenderModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FWalkLoopExtenderModule::RegisterMenus));
    }

    virtual void ShutdownModule() override
    {
        if (UToolMenus::IsToolMenuUIEnabled())
        {
            UToolMenus::UnRegisterStartupCallback(this);
            UToolMenus::UnregisterOwner(this);
        }
    }

private:
    void RegisterMenus()
    {
        FToolMenuOwnerScoped OwnerScoped(this);

        if (UToolMenu* ToolsMenu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools"))
        {
            FToolMenuSection& Section = ToolsMenu->FindOrAddSection("WalkLoopExtender");
            Section.AddMenuEntry(
                "WalkLoopExtender_Create600FrameWalkLoop",
                LOCTEXT("Create600FrameWalkLoop_Label", "Create 600-Frame Walk Loop"),
                LOCTEXT("Create600FrameWalkLoop_Tooltip", "Create a longer continuous root-motion walk loop from the selected animation sequence."),
                FSlateIcon(),
                FUIAction(
                    FExecuteAction::CreateRaw(this, &FWalkLoopExtenderModule::ExtendSelectedAnimations),
                    FCanExecuteAction::CreateRaw(this, &FWalkLoopExtenderModule::HasSelectedAnimation)));
        }

        if (UToolMenu* AnimMenu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetContextMenu.AnimSequence"))
        {
            FToolMenuSection& Section = AnimMenu->FindOrAddSection("WalkLoopExtender");
            Section.AddMenuEntry(
                "WalkLoopExtender_Create600FrameWalkLoop_Context",
                LOCTEXT("Create600FrameWalkLoop_ContextLabel", "Create 600-Frame Walk Loop"),
                LOCTEXT("Create600FrameWalkLoop_ContextTooltip", "Duplicate this animation and extend it into a continuous 600-frame walk."),
                FSlateIcon(),
                FUIAction(
                    FExecuteAction::CreateRaw(this, &FWalkLoopExtenderModule::ExtendSelectedAnimations),
                    FCanExecuteAction::CreateRaw(this, &FWalkLoopExtenderModule::HasSelectedAnimation)));
        }
    }

    bool HasSelectedAnimation() const
    {
        TArray<UAnimSequence*> SelectedAnimations;
        GetSelectedAnimations(SelectedAnimations);
        return SelectedAnimations.Num() > 0;
    }

    void GetSelectedAnimations(TArray<UAnimSequence*>& OutAnimations) const
    {
        OutAnimations.Reset();

        FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
        TArray<FAssetData> SelectedAssets;
        ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

        for (const FAssetData& AssetData : SelectedAssets)
        {
            if (UAnimSequence* Animation = Cast<UAnimSequence>(AssetData.GetAsset()))
            {
                OutAnimations.Add(Animation);
            }
        }
    }

    void ExtendSelectedAnimations()
    {
        TArray<UAnimSequence*> SelectedAnimations;
        GetSelectedAnimations(SelectedAnimations);

        if (SelectedAnimations.Num() == 0)
        {
            ShowNotification(LOCTEXT("NoAnimationSelected", "Select an AnimSequence first."), SNotificationItem::CS_Fail);
            return;
        }

        const FWalkLoopExtendOptions Options = UWalkLoopExtenderLibrary::MakeOptionsFromSettings();

        int32 SuccessCount = 0;
        FString LastMessage;

        for (UAnimSequence* Animation : SelectedAnimations)
        {
            UAnimSequence* OutputAnimation = nullptr;
            FString Message;
            if (UWalkLoopExtenderLibrary::CreateExtendedWalkLoop(Animation, Options, OutputAnimation, Message))
            {
                ++SuccessCount;
            }

            LastMessage = Message;
        }

        if (SuccessCount > 0)
        {
            const FText SuccessText = FText::Format(
                LOCTEXT("WalkLoopCreated", "Created {0} extended walk loop(s). {1}"),
                FText::AsNumber(SuccessCount),
                FText::FromString(LastMessage));
            ShowNotification(SuccessText, SNotificationItem::CS_Success);
        }
        else
        {
            ShowNotification(FText::FromString(LastMessage.IsEmpty() ? TEXT("Could not create walk loop.") : LastMessage), SNotificationItem::CS_Fail);
        }
    }

    void ShowNotification(const FText& Message, SNotificationItem::ECompletionState State) const
    {
        FNotificationInfo Info(Message);
        Info.ExpireDuration = 7.0f;
        Info.bUseLargeFont = false;

        const TSharedPtr<SNotificationItem> Notification = FSlateNotificationManager::Get().AddNotification(Info);
        if (Notification.IsValid())
        {
            Notification->SetCompletionState(State);
        }
    }
};

IMPLEMENT_MODULE(FWalkLoopExtenderModule, WalkLoopExtender)

#undef LOCTEXT_NAMESPACE
