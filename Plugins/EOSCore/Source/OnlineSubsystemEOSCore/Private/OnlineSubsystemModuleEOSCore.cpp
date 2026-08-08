/**
* Copyright (C) 2017-2025 | eelDev AB
*
* EOSCore Documentation: https://eeldev.com
*/

#include "OnlineSubsystemModuleEOSCore.h"
#include "OnlineSubsystemModule.h"
#include "OnlineSubsystemEOSCore.h"
#include "OnlineSubsystemEOSCorePrivatePCH.h"

#include "OnlineSessionEOSCore.h"

#if PLATFORM_SWITCH
#include <Switch/eos_Switch.h>
#endif

#define LOCTEXT_NAMESPACE "EOS"

DEFINE_LOG_CATEGORY(LogEOSCoreSubsystem);
IMPLEMENT_MODULE(FOnlineSubsystemEOSCoreModule, OnlineSubsystemEOSCore);

class FOnlineFactoryEOSCore : public IOnlineFactory
{
public:
	FOnlineFactoryEOSCore()
	{
	}

	virtual ~FOnlineFactoryEOSCore()
	{
	}

public:
	virtual IOnlineSubsystemPtr CreateSubsystem(FName InstanceName)
	{
		LogEOSVerbose("");
		
		FOnlineSubsystemEOSCorePtr OnlineSub = MakeShared<FOnlineSubsystemEOSCore, ESPMode::ThreadSafe>(InstanceName);

		if (OnlineSub && OnlineSub->IsEnabled())
		{
			if (!OnlineSub->Init())
			{
				LogEOSError("EOS SUBSYSTEM failed to initialize!");
				OnlineSub->Shutdown();
				OnlineSub = nullptr;
			}
		}
		else
		{
			LogEOSVerbose("EOS SUBSYSTEM is not enabled, disabling OSS.");

			if (OnlineSub.IsValid())
			{
				OnlineSub->Shutdown();
				OnlineSub = nullptr;
			}
		}

		return OnlineSub;
	}
};

void FOnlineSubsystemEOSCoreModule::StartupModule()
{
	LogEOSVerbose("");

#if WITH_EDITOR
	auto& PluginManager = IPluginManager::Get();
	
	if (TSharedPtr<IPlugin> EOSSharedPtr = PluginManager.FindPlugin("EOSShared"))
	{
		if (EOSSharedPtr->IsEnabled())
		{
			UE_LOG(LogTemp, Warning, TEXT("EOSShared Plugin is ENABLED. Make sure you set [EOSSDK] bHasProjectBinary=true in your DefaultEngine.ini"));
		}
	}

	if (TSharedPtr<IPlugin> EOSSharedPtr = PluginManager.FindPlugin("EOSVoiceChat"))
	{
		if (EOSSharedPtr->IsEnabled())
		{
			UE_LOG(LogTemp, Warning, TEXT("EOSVoiceChat Plugin is ENABLED. Make sure you set [EOSSDK] bHasProjectBinary=true in your DefaultEngine.ini"));
		}
	}

	if (TSharedPtr<IPlugin> EOSSharedPtr = PluginManager.FindPlugin("SocketSubsystemEOS"))
	{
		if (EOSSharedPtr->IsEnabled())
		{
			UE_LOG(LogTemp, Warning, TEXT("SocketSubsystemEOS Plugin is ENABLED. Make sure you set [EOSSDK] bHasProjectBinary=true in your DefaultEngine.ini"));
		}
	}

	if (TSharedPtr<IPlugin> EOSSharedPtr = PluginManager.FindPlugin("OnlineSubsystemEOS"))
	{
		if (EOSSharedPtr->IsEnabled())
		{
			UE_LOG(LogTemp, Warning, TEXT("OnlineSubsystemEOS Plugin is ENABLED. Make sure you set [EOSSDK] bHasProjectBinary=true in your DefaultEngine.ini"));
		}
	}
#endif
	
	m_EOSFactory = new FOnlineFactoryEOSCore();

	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.RegisterPlatformService(EOSCORE_SUBSYSTEM, m_EOSFactory);

	const FString BaseDir = IPluginManager::Get().FindPlugin("EOSCore")->GetBaseDir();
	
	FString LibraryPath;
#if PLATFORM_WINDOWS
	LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/EOSLibrary/Bin/EOSSDK-Win64-Shipping.dll"));
#elif PLATFORM_LINUX
	LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/EOSLibrary/Bin/libEOSSDK-Linux-Shipping.so"));
#elif PLATFORM_MAC
	LibraryPath = FPaths::Combine(*BaseDir, TEXT("Source/ThirdParty/EOSLibrary/Bin/libEOSSDK-Mac-Shipping.dylib"));
#elif PLATFORM_ANDROID
	LibraryPath = FString("libEOSSDK.so");
#elif PLATFORM_IOS
	LibraryPath.Empty();
#endif

	m_EOSHandle = !LibraryPath.IsEmpty() ? FPlatformProcess::GetDllHandle(*LibraryPath) : nullptr;

	if (!m_EOSHandle && !LibraryPath.IsEmpty())
	{
		FMessageDialog::Open(EAppMsgType::Ok, LOCTEXT("ThirdPartyLibraryError", "Failed to load EOS third party library"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("EOS SDK Loaded!"));
	}

	FOnlineSubsystemEOSCore::ModuleInit();
}

void FOnlineSubsystemEOSCoreModule::ShutdownModule()
{
	LogEOSVerbose("");
	
	FOnlineSubsystemModule& OSS = FModuleManager::GetModuleChecked<FOnlineSubsystemModule>("OnlineSubsystem");
	OSS.UnregisterPlatformService(EOSCORE_SUBSYSTEM);

	delete m_EOSFactory;
	m_EOSFactory = nullptr;
}

#undef LOCTEXT_NAMESPACE

static FDelegateHandle JoinSessionCallbackDelegate;
static FOnSessionUserInviteAcceptedDelegate OnSessionUserInviteAcceptedDelegate;
static FDelegateHandle OnSessionUserInviteAcceptedDelegateHandle;

void UEOSSubsystemLibrary::ListenForSessionInvites(APlayerController* playerController, const FOnSessionInviteCallback& callback)
{
	LogEOSVerbose("");
	
	if (playerController)
	{
		if (FOnlineSubsystemEOSCore* m_Subsystem = static_cast<FOnlineSubsystemEOSCore*>(Online::GetSubsystem(GEngine->GetWorldFromContextObject(playerController, EGetWorldErrorMode::ReturnNull), EOSCORE_SUBSYSTEM)))
		{
			if (m_Subsystem->m_SessionInterfacePtr)
			{
				OnSessionUserInviteAcceptedDelegate = FOnSessionUserInviteAcceptedDelegate::CreateLambda([m_Subsystem, callback, playerController](bool bWasSuccessful, int32 ControllerId, TSharedPtr<const FUniqueNetId> UserId, const FOnlineSessionSearchResult& SearchResult)
				{
					if (SearchResult.IsValid())
					{
						FBlueprintSessionResult BlueprintResult;
						BlueprintResult.OnlineResult = SearchResult;

						callback.ExecuteIfBound(true, playerController, BlueprintResult);
					}
					else
					{
						callback.ExecuteIfBound(false, playerController, FBlueprintSessionResult());
					}
				});

				OnSessionUserInviteAcceptedDelegateHandle = m_Subsystem->m_SessionInterfacePtr->AddOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegate);
			}
		}
	}
}

void UEOSSubsystemLibrary::StopListeningForSessionInvites(APlayerController* playerController)
{
	LogEOSVerbose("");
	
	if (playerController)
	{
		if (FOnlineSubsystemEOSCore* m_Subsystem = static_cast<FOnlineSubsystemEOSCore*>(Online::GetSubsystem(GEngine->GetWorldFromContextObject(playerController, EGetWorldErrorMode::ReturnNull), EOSCORE_SUBSYSTEM)))
		{
			if (m_Subsystem->m_SessionInterfacePtr)
			{
				m_Subsystem->m_SessionInterfacePtr->ClearOnSessionUserInviteAcceptedDelegate_Handle(OnSessionUserInviteAcceptedDelegateHandle);
			}
		}
	}
}
