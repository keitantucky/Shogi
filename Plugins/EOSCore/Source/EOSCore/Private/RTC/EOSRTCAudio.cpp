/**
* Copyright (C) 2017-2025 | eelDev AB
*
* EOSCore Documentation: https://eeldev.com
*/

#include "RTC/EOSRTCAudio.h"
#include "Core/EOSCorePluginPrivatePCH.h"
#include "Core/EOSCoreLogging.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
//		UCoreRTCAudio
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

void UCoreRTCAudio::Deinitialize()
{
	LogEOSVerbose("");

	for (auto Element : m_OnAudioDevicesChangedCallbacks)
	{
		EOS_RTCAudio_RemoveNotifyAudioDevicesChanged(Element.Value->m_AudioHandle, Element.Key);
	}

	for (auto Element : m_OnAudioInputStateCallbacks)
	{
		EOS_RTCAudio_RemoveNotifyAudioInputState(Element.Value->m_AudioHandle, Element.Key);
	}

	for (auto Element : m_OnAudioOutputStateCallbacks)
	{
		EOS_RTCAudio_RemoveNotifyAudioOutputState(Element.Value->m_AudioHandle, Element.Key);
	}

	for (auto Element : m_OnAudioBeforeSendCallbacks)
	{
		EOS_RTCAudio_RemoveNotifyAudioBeforeSend(Element.Value->m_AudioHandle, Element.Key);
	}

	for (auto Element : m_OnAudioBeforeRenderCallbacks)
	{
		EOS_RTCAudio_RemoveNotifyAudioBeforeRender(Element.Value->m_AudioHandle, Element.Key);
	}

	for (auto Element : m_OnParticipantUpdatedCallbacks)
	{
		EOS_RTCAudio_RemoveNotifyParticipantUpdated(Element.Value->m_AudioHandle, Element.Key);
	}

	m_OnAudioDevicesChangedCallbacks.Empty();
	m_OnAudioInputStateCallbacks.Empty();
	m_OnAudioOutputStateCallbacks.Empty();
	m_OnAudioBeforeSendCallbacks.Empty();
	m_OnAudioBeforeRenderCallbacks.Empty();
	m_OnParticipantUpdatedCallbacks.Empty();
}

UCoreRTCAudio* UCoreRTCAudio::GetRTCAudio(UObject* WorldContextObject)
{
	if (WorldContextObject)
	{
		if (UWorld* World = WorldContextObject->GetWorld())
		{
			if (UGameInstance* GameInstance = World->GetGameInstance())
			{
				return GameInstance->GetSubsystem<UCoreRTCAudio>();
			}
		}
	}
	return nullptr;
}

EOSResult UCoreRTCAudio::EOSRTCAudioRegisterPlatformAudioUser(FEOSHRTCAudio Handle, FEOSRegisterPlatformAudioUserOptions Options)
{
	LogEOSVerbose("");

	EOSResult Result = EOSResult::EOS_UnexpectedError;

	if (Handle)
	{
		const FRegisterPlatformAudioUserOptions Parameters(Options.UserId);

		Result = EOSHelpers::ToEOSCoreResult(EOS_RTCAudio_RegisterPlatformAudioUser(Handle, &Parameters));
	}

	return Result;
}

EOSResult UCoreRTCAudio::EOSRTCAudioUnregisterPlatformAudioUser(FEOSHRTCAudio Handle, FEOSUnregisterPlatformAudioUserOptions Options)
{
	LogEOSVerbose("");

	EOSResult Result = EOSResult::EOS_UnexpectedError;

	if (Handle)
	{
		const FUnregisterPlatformAudioUserOptions Parameters(Options.UserId);

		Result = EOSHelpers::ToEOSCoreResult(EOS_RTCAudio_UnregisterPlatformAudioUser(Handle, &Parameters));
	}

	return Result;
}

int32 UCoreRTCAudio::EOSRTCAudioGetAudioInputDevicesCount(FEOSHRTCAudio Handle, const FEOSGetAudioInputDevicesCountOptions Options)
{
	LogEOSVeryVerbose("");

	int32 Result = 0;

	if (Handle)
	{
		EOS_RTCAudio_GetAudioInputDevicesCountOptions Parameters = {};
		Parameters.ApiVersion = EOS_RTCAUDIO_GETAUDIOINPUTDEVICESCOUNT_API_LATEST;

		Result = EOS_RTCAudio_GetAudioInputDevicesCount(Handle, &Parameters);
	}

	return Result;
}

FEOSAudioInputDeviceInfo UCoreRTCAudio::EOSRTCAudioGetAudioInputDeviceByIndex(FEOSHRTCAudio Handle, FEOSGetAudioOutputDeviceByIndexOptions Options)
{
	LogEOSVeryVerbose("");

	FEOSAudioInputDeviceInfo Result;

	if (Handle)
	{
		EOS_RTCAudio_GetAudioInputDeviceByIndexOptions Parameters = {};
		Parameters.ApiVersion = EOS_RTCAUDIO_GETAUDIOINPUTDEVICEBYINDEX_API_LATEST;
		Parameters.DeviceInfoIndex = Options.DeviceInfoIndex;

		if (const EOS_RTCAudio_AudioInputDeviceInfo* Data = EOS_RTCAudio_GetAudioInputDeviceByIndex(Handle, &Parameters))
		{
			Result = Data;
		}
	}

	return Result;
}

int32 UCoreRTCAudio::EOSRTCAudioGetAudioOutputDevicesCount(FEOSHRTCAudio Handle, FEOSGetAudioOutputDevicesCountOptions Options)
{
	LogEOSVeryVerbose("");

	int32 Result = 0;

	if (Handle)
	{
		EOS_RTCAudio_GetAudioOutputDevicesCountOptions Parameters = {};
		Parameters.ApiVersion = EOS_RTCAUDIO_GETAUDIOOUTPUTDEVICESCOUNT_API_LATEST;

		Result = EOS_RTCAudio_GetAudioOutputDevicesCount(Handle, &Parameters);
	}

	return Result;
}

FEOSAudioOutputDeviceInfo UCoreRTCAudio::EOSRTCAudioGetAudioOutputDeviceByIndex(FEOSHRTCAudio Handle, FEOSGetAudioOutputDeviceByIndexOptions Options)
{
	LogEOSVeryVerbose("");

	FEOSAudioOutputDeviceInfo Result;

	if (Handle)
	{
		EOS_RTCAudio_GetAudioOutputDeviceByIndexOptions Parameters = {};
		Parameters.ApiVersion = EOS_RTCAUDIO_GETAUDIOOUTPUTDEVICEBYINDEX_API_LATEST;
		Parameters.DeviceInfoIndex = Options.DeviceInfoIndex;

		Result = *EOS_RTCAudio_GetAudioOutputDeviceByIndex(Handle, &Parameters);
	}

	return Result;
}

EOSResult UCoreRTCAudio::EOSRTCAudioSetAudioInputSettings(FEOSHRTCAudio Handle, FEOSSetAudioInputSettingsOptions Options)
{
	LogEOSVerbose("");

	EOSResult Result = EOSResult::EOS_UnexpectedError;

	if (Handle)
	{
		FSetAudioInputSettingsOptions Parameters(Options.DeviceId);
		Parameters.LocalUserId = Options.LocalUserId;
		Parameters.Volume = Options.Volume;
		Parameters.bPlatformAEC = Options.bPlatformAEC;

		Result = EOSHelpers::ToEOSCoreResult(EOS_RTCAudio_SetAudioInputSettings(Handle, &Parameters));
	}

	return Result;
}

EOSResult UCoreRTCAudio::EOSRTCAudioSetAudioOutputSettings(FEOSHRTCAudio Handle, FEOSSetAudioOutputSettingsOptions Options)
{
	LogEOSVerbose("");

	EOSResult Result = EOSResult::EOS_UnexpectedError;

	if (Handle)
	{
		FSetAudioOutputSettingsOptions Parameters(Options.DeviceId);
		Parameters.LocalUserId = Options.LocalUserId;
		Parameters.Volume = Options.Volume;

		Result = EOSHelpers::ToEOSCoreResult(EOS_RTCAudio_SetAudioOutputSettings(Handle, &Parameters));
	}

	return Result;
}

EOSResult UCoreRTCAudio::EOSRTCAudioSendAudio(FEOSHRTCAudio Handle, FEOSSendAudioOptions Options)
{
	LogEOSVeryVerbose("");

	EOSResult Result = EOSResult::EOS_UnexpectedError;

	if (Handle)
	{
		FSendAudioOptions Parameters(Options.RoomName);
		Parameters.LocalUserId = Options.LocalUserId;
		const EOS_RTCAudio_AudioBuffer* Buffer = Options.Buffer;
		Parameters.Buffer = const_cast<EOS_RTCAudio_AudioBuffer*>(Buffer);

		Result = EOSHelpers::ToEOSCoreResult(EOS_RTCAudio_SendAudio(Handle, &Parameters));
	}

	return Result;
}

void UCoreRTCAudio::EOSRTCAudioUpdateSending(FEOSHRTCAudio Handle, FEOSUpdateSendingOptions Options, const FOnUpdateSendingCallback& Callback)
{
	LogEOSVerbose("");

	if (Handle)
	{
		FUpdateSendingOptions Parameters(Options.RoomName);
		Parameters.LocalUserId = Options.LocalUserId;
		Parameters.AudioStatus = static_cast<EOS_ERTCAudioStatus>(Options.AudioStatus);

		FUpdateSendingOptionsCallback* CallbackObj = new FUpdateSendingOptionsCallback(Callback);
		EOS_RTCAudio_UpdateSending(Handle, &Parameters, CallbackObj, Internal_OnUpdateSendingCallback);
	}
}

void UCoreRTCAudio::EOSRTCAudioUpdateReceiving(FEOSHRTCAudio Handle, FEOSUpdateReceivingOptions Options, const FOnUpdateReceivingCallback& Callback)
{
	LogEOSVerbose("");

	if (Handle)
	{
		FUpdateReceivingOptions Parameters(Options.RoomName);
		Parameters.LocalUserId = Options.LocalUserId;
		Parameters.ParticipantId = Options.ParticipantId;
		Parameters.bAudioEnabled = Options.bAudioEnabled;

		FUpdateReceivingOptionsCallback* CallbackObj = new FUpdateReceivingOptionsCallback(Callback);
		EOS_RTCAudio_UpdateReceiving(Handle, &Parameters, CallbackObj, Internal_OnUpdateReceivingCallback);
	}
}

void UCoreRTCAudio::EOSRTCAudioUpdateSendingVolume(FEOSHRTCAudio Handle, FEOSRTCAudioUpdateSendingVolumeOptions Options, const FOnUpdateSendingVolumeCallback& Callback)
{
	LogEOSVerbose("");

	if (Handle)
	{
		EOS_RTCAudio_UpdateSendingVolumeOptions Parameters = {};
		Parameters.ApiVersion = EOS_RTCAUDIO_UPDATESENDINGVOLUME_API_LATEST;
		Parameters.LocalUserId = Options.LocalUserId;
		const FTCHARToUTF8 ConvertedRoomName(*Options.RoomName);
		Parameters.RoomName = ConvertedRoomName.Get();
		Parameters.Volume = Options.Volume;

		FUpdateSendingVolumeCallback* CallbackObj = new FUpdateSendingVolumeCallback(Callback);
		EOS_RTCAudio_UpdateSendingVolume(Handle, &Parameters, CallbackObj, Internal_UpdateSendingVolumeCallback);
	}
}

void UCoreRTCAudio::EOSRTCAudioUpdateReceivingVolume(FEOSHRTCAudio Handle, FEOSRTCAudioUpdateReceivingVolumeOptions Options, const FOnUpdateReceivingVolumeCallback& Callback)
{
	LogEOSVerbose("");

	if (Handle)
	{
		EOS_RTCAudio_UpdateReceivingVolumeOptions Parameters = {};
		Parameters.ApiVersion = EOS_RTCAUDIO_UPDATERECEIVINGVOLUME_API_LATEST;
		Parameters.LocalUserId = Options.LocalUserId;
		const FTCHARToUTF8 ConvertedRoomName(*Options.RoomName);
		Parameters.RoomName = ConvertedRoomName.Get();
		Parameters.Volume = Options.Volume;

		FUpdateReceivingVolumeCallback* CallbackObj = new FUpdateReceivingVolumeCallback(Callback);
		EOS_RTCAudio_UpdateReceivingVolume(Handle, &Parameters, CallbackObj, Internal_UpdateReceivingVolumeCallback);
	}
}

void UCoreRTCAudio::EOSRTCAudioUpdateParticipantVolume(FEOSHRTCAudio Handle, FEOSRTCAudioUpdateParticipantVolumeOptions Options, const FOnUpdateParticipantVolumeCallback& Callback)
{
	LogEOSVerbose("");

	if (Handle)
	{
		EOS_RTCAudio_UpdateParticipantVolumeOptions Parameters = {};
		Parameters.ApiVersion = EOS_RTCAUDIO_UPDATEPARTICIPANTVOLUME_API_LATEST;
		Parameters.LocalUserId = Options.LocalUserId;
		const FTCHARToUTF8 ConvertedRoomName(*Options.RoomName);
		Parameters.RoomName = ConvertedRoomName.Get();
		Parameters.ParticipantId = Options.ParticipantId;
		Parameters.Volume = Options.Volume;

		FUpdateParticipantVolumeCallback* CallbackObj = new FUpdateParticipantVolumeCallback(Callback);
		EOS_RTCAudio_UpdateParticipantVolume(Handle, &Parameters, CallbackObj, Internal_UpdateParticipantVolumeCallback);
	}
}

FEOSNotificationId UCoreRTCAudio::EOSRTCAudioAddNotifyParticipantUpdated(UObject* WorldContextObject, FEOSHRTCAudio Handle, FEOSAddNotifyParticipantUpdatedOptions Options, const FOnParticipantUpdatedCallback& Callback)
{
	LogEOSVerbose("");

	FEOSNotificationId NotificationId;

	if (Handle)
	{
		FAddNotifyParticipantUpdatedOptions Parameters(Options.RoomName);
		Parameters.LocalUserId = Options.LocalUserId;

		FParticipantUpdatedCallback* CallbackObj = new FParticipantUpdatedCallback(Handle, Callback);

		NotificationId = EOS_RTCAudio_AddNotifyParticipantUpdated(Handle, &Parameters, CallbackObj, ([](const EOS_RTCAudio_ParticipantUpdatedCallbackInfo* Data)
		{
			const FParticipantUpdatedCallback* CallbackData = static_cast<FParticipantUpdatedCallback*>(Data->ClientData);
			CallbackData->m_Callback.ExecuteIfBound(*Data);
		}));
		GetRTCAudio(WorldContextObject)->m_OnParticipantUpdatedCallbacks.Add(NotificationId, CallbackObj);
	}

	return NotificationId;
}

void UCoreRTCAudio::EOSRTCAudioRemoveNotifyParticipantUpdated(UObject* WorldContextObject, FEOSHRTCAudio Handle, FEOSNotificationId NotificationId)
{
	LogEOSVerbose("");

	if (Handle)
	{
		EOS_RTCAudio_RemoveNotifyParticipantUpdated(Handle, NotificationId);
		GetRTCAudio(WorldContextObject)->m_OnParticipantUpdatedCallbacks.Remove(NotificationId);
	}
}

FEOSNotificationId UCoreRTCAudio::EOSRTCAudioAddNotifyAudioDevicesChanged(UObject* WorldContextObject, FEOSHRTCAudio Handle, FEOSAddNotifyAudioDevicesChangedOptions Options, const FOnAudioDevicesChangedCallback& Callback)
{
	LogEOSVerbose("");

	FEOSNotificationId NotificationId;

	if (Handle)
	{
		EOS_RTCAudio_AddNotifyAudioDevicesChangedOptions Parameters;
		Parameters.ApiVersion = EOS_RTCAUDIO_ADDNOTIFYAUDIODEVICESCHANGED_API_LATEST;

		FAudioDevicesChangedCallback* CallbackObj = new FAudioDevicesChangedCallback(Handle, Callback);

		NotificationId = EOS_RTCAudio_AddNotifyAudioDevicesChanged(Handle, &Parameters, CallbackObj, ([](const EOS_RTCAudio_AudioDevicesChangedCallbackInfo* Data)
		{
			const FAudioDevicesChangedCallback* CallbackData = static_cast<FAudioDevicesChangedCallback*>(Data->ClientData);
			CallbackData->m_Callback.ExecuteIfBound(*Data);
		}));
		GetRTCAudio(WorldContextObject)->m_OnAudioDevicesChangedCallbacks.Add(NotificationId, CallbackObj);
	}

	return NotificationId;
}

void UCoreRTCAudio::EOSRTCAudioRemoveNotifyAudioDevicesChanged(UObject* WorldContextObject, FEOSHRTCAudio Handle, FEOSNotificationId NotificationId)
{
	LogEOSVerbose("");

	if (Handle)
	{
		EOS_RTCAudio_RemoveNotifyAudioDevicesChanged(Handle, NotificationId);
		GetRTCAudio(WorldContextObject)->m_OnAudioDevicesChangedCallbacks.Remove(NotificationId);
	}
}

FEOSNotificationId UCoreRTCAudio::EOSRTCAudioAddNotifyAudioInputState(UObject* WorldContextObject, FEOSHRTCAudio Handle, FEOSAddNotifyAudioInputStateOptions Options, const FOnAudioInputStateCallback& Callback)
{
	LogEOSVerbose("");

	FEOSNotificationId NotificationId;

	if (Handle)
	{
		FAddNotifyAudioInputStateOptions Parameters(Options.RoomName);
		Parameters.LocalUserId = Options.LocalUserId;

		FAudioInputStateCallback* CallbackObj = new FAudioInputStateCallback(Handle, Callback);

		NotificationId = EOS_RTCAudio_AddNotifyAudioInputState(Handle, &Parameters, CallbackObj, ([](const EOS_RTCAudio_AudioInputStateCallbackInfo* Data)
		{
			const FAudioInputStateCallback* CallbackData = static_cast<FAudioInputStateCallback*>(Data->ClientData);
			CallbackData->m_Callback.ExecuteIfBound(*Data);
		}));
		GetRTCAudio(WorldContextObject)->m_OnAudioInputStateCallbacks.Add(NotificationId, CallbackObj);
	}

	return NotificationId;
}

void UCoreRTCAudio::EOSRTCAudioRemoveNotifyAudioInputState(UObject* WorldContextObject, FEOSHRTCAudio Handle, FEOSNotificationId NotificationId)
{
	LogEOSVerbose("");

	if (Handle)
	{
		EOS_RTCAudio_RemoveNotifyAudioInputState(Handle, NotificationId);
		GetRTCAudio(WorldContextObject)->m_OnAudioInputStateCallbacks.Remove(NotificationId);
	}
}

FEOSNotificationId UCoreRTCAudio::EOSRTCAudioAddNotifyAudioOutputState(UObject* WorldContextObject, FEOSHRTCAudio Handle, FEOSAddNotifyAudioOutputStateOptions Options, const FOnAudioOutputStateCallback& Callback)
{
	LogEOSVerbose("");

	FEOSNotificationId NotificationId;

	if (Handle)
	{
		FAddNotifyAudioOutputStateOptions Parameters(Options.RoomName);
		Parameters.LocalUserId = Options.LocalUserId;

		FAudioOutputStateCallback* CallbackObj = new FAudioOutputStateCallback(Handle, Callback);

		NotificationId = EOS_RTCAudio_AddNotifyAudioOutputState(Handle, &Parameters, CallbackObj, ([](const EOS_RTCAudio_AudioOutputStateCallbackInfo* Data)
		{
			const FAudioOutputStateCallback* CallbackData = static_cast<FAudioOutputStateCallback*>(Data->ClientData);
			CallbackData->m_Callback.ExecuteIfBound(*Data);
		}));
		GetRTCAudio(WorldContextObject)->m_OnAudioOutputStateCallbacks.Add(NotificationId, CallbackObj);
	}

	return NotificationId;
}

void UCoreRTCAudio::EOSRTCAudioRemoveNotifyAudioOutputState(UObject* WorldContextObject, FEOSHRTCAudio Handle, FEOSNotificationId NotificationId)
{
	LogEOSVerbose("");

	if (Handle)
	{
		EOS_RTCAudio_RemoveNotifyAudioOutputState(Handle, NotificationId);
		GetRTCAudio(WorldContextObject)->m_OnAudioOutputStateCallbacks.Remove(NotificationId);
	}
}

FEOSNotificationId UCoreRTCAudio::EOSRTCAudioAddNotifyAudioBeforeSend(UObject* WorldContextObject, FEOSHRTCAudio Handle, FEOSAddNotifyAudioBeforeSendOptions Options, const FOnAudioBeforeSendCallback& Callback)
{
	LogEOSVerbose("");

	FEOSNotificationId NotificationId;

	if (Handle)
	{
		FAddNotifyAudioBeforeSendOptions Parameters(Options.RoomName);
		Parameters.LocalUserId = Options.LocalUserId;

		FAudioBeforeSendCallback* CallbackObj = new FAudioBeforeSendCallback(Handle, Callback);

		NotificationId = EOS_RTCAudio_AddNotifyAudioBeforeSend(Handle, &Parameters, CallbackObj, ([](const EOS_RTCAudio_AudioBeforeSendCallbackInfo* Data)
		{
			const FAudioBeforeSendCallback* CallbackData = static_cast<FAudioBeforeSendCallback*>(Data->ClientData);
			CallbackData->m_Callback.ExecuteIfBound(*Data);
		}));
		GetRTCAudio(WorldContextObject)->m_OnAudioBeforeSendCallbacks.Add(NotificationId, CallbackObj);
	}

	return NotificationId;
}

void UCoreRTCAudio::EOSRTCAudioRemoveNotifyAudioBeforeSend(UObject* WorldContextObject, FEOSHRTCAudio Handle, FEOSNotificationId NotificationId)
{
	LogEOSVerbose("");

	if (Handle)
	{
		EOS_RTCAudio_RemoveNotifyAudioBeforeSend(Handle, NotificationId);
		GetRTCAudio(WorldContextObject)->m_OnAudioBeforeSendCallbacks.Remove(NotificationId);
	}
}

FEOSNotificationId UCoreRTCAudio::EOSRTCAudioAddNotifyAudioBeforeRender(UObject* WorldContextObject, FEOSHRTCAudio Handle, FEOSAddNotifyAudioBeforeRenderOptions Options, const FOnAudioBeforeRenderCallback& Callback)
{
	LogEOSVerbose("");

	FEOSNotificationId NotificationId;

	if (Handle)
	{
		FAddNotifyAudioBeforeRenderOptions Parameters(Options.RoomName);
		Parameters.LocalUserId = Options.LocalUserId;
		Parameters.bUnmixedAudio = Options.bUnmixedAudio;

		FAudioBeforeRenderCallback* CallbackObj = new FAudioBeforeRenderCallback(Handle, Callback);

		NotificationId = EOS_RTCAudio_AddNotifyAudioBeforeRender(Handle, &Parameters, CallbackObj, ([](const EOS_RTCAudio_AudioBeforeRenderCallbackInfo* Data)
		{
			const FAudioBeforeRenderCallback* CallbackData = static_cast<FAudioBeforeRenderCallback*>(Data->ClientData);
			CallbackData->m_Callback.ExecuteIfBound(*Data);
		}));
		GetRTCAudio(WorldContextObject)->m_OnAudioBeforeRenderCallbacks.Add(NotificationId, CallbackObj);
	}

	return NotificationId;
}

void UCoreRTCAudio::EOSRTCAudioRemoveNotifyAudioBeforeRender(UObject* WorldContextObject, FEOSHRTCAudio Handle, FEOSNotificationId NotificationId)
{
	LogEOSVerbose("");

	if (Handle)
	{
		EOS_RTCAudio_RemoveNotifyAudioBeforeRender(Handle, NotificationId);
		GetRTCAudio(WorldContextObject)->m_OnAudioBeforeRenderCallbacks.Remove(NotificationId);
	}
}

void UCoreRTCAudio::EOSRTCAudioRegisterPlatformUser(FEOSHRTCAudio Handle, FEOSRTCAudioRegisterPlatformUserOptions Options, const FOnRegisterPlatformUserCallback& Callback)
{
	LogEOSVerbose("");

	if (Handle)
	{
		EOS_RTCAudio_RegisterPlatformUserOptions Parameters = {};
		Parameters.ApiVersion = EOS_RTCAUDIO_REGISTERPLATFORMUSER_API_LATEST;

		const FTCHARToUTF8 ConvertedPlatformUserId(*Options.PlatformUserId);
		Parameters.PlatformUserId = ConvertedPlatformUserId.Get();
		
		FRegisterPlatformUserCallback* CallbackObj = new FRegisterPlatformUserCallback(Callback);
		EOS_RTCAudio_RegisterPlatformUser(Handle, &Parameters, CallbackObj, Internal_RegisterPlatformUserCallback);
	}
}

void UCoreRTCAudio::EOSRTCAudioUnregisterPlatformUser(FEOSHRTCAudio Handle, FEOSRTCAudioUnregisterPlatformUserOptions Options, const FOnUnregisterPlatformUserCallback& Callback)
{
	LogEOSVerbose("");

	if (Handle)
	{
		EOS_RTCAudio_UnregisterPlatformUserOptions Parameters = {};
		Parameters.ApiVersion = EOS_RTCAUDIO_UNREGISTERPLATFORMUSER_API_LATEST;

		const FTCHARToUTF8 ConvertedPlatformUserId(*Options.PlatformUserId);
		Parameters.PlatformUserId = ConvertedPlatformUserId.Get();
		
		FUnregisterPlatformUserCallback* CallbackObj = new FUnregisterPlatformUserCallback(Callback);
		EOS_RTCAudio_UnregisterPlatformUser(Handle, &Parameters, CallbackObj, Internal_UnregisterPlatformUserCallback);
	}
}

void UCoreRTCAudio::EOSRTCAudioQueryInputDevicesInformation(FEOSHRTCAudio Handle, const FOnQueryInputDevicesInformationCallback& Callback)
{
	LogEOSVerbose("");

	if (Handle)
	{
		EOS_RTCAudio_QueryInputDevicesInformationOptions Parameters = {};
		Parameters.ApiVersion = EOS_RTCAUDIO_QUERYINPUTDEVICESINFORMATION_API_LATEST;

		FQueryInputDevicesInformationCallback* CallbackObj = new FQueryInputDevicesInformationCallback(Callback);
		EOS_RTCAudio_QueryInputDevicesInformation(Handle, &Parameters, CallbackObj, Internal_QueryInputDevicesInformationCallback);
	}
}

int32 UCoreRTCAudio::EOSRTCAudioGetInputDevicesCount(FEOSHRTCAudio Handle)
{
	LogEOSVerbose("");

	int32 Result = 0;

	if (Handle)
	{
		EOS_RTCAudio_GetInputDevicesCountOptions Parameters = {};
		Parameters.ApiVersion = EOS_RTCAUDIO_GETINPUTDEVICESCOUNT_API_LATEST;

		Result = EOS_RTCAudio_GetInputDevicesCount(Handle, &Parameters);
	}

	return Result;
}

EOSResult UCoreRTCAudio::EOSRTCAudioCopyInputDeviceInformationByIndex(FEOSHRTCAudio Handle, FEOSRTCAudioCopyInputDeviceInformationByIndexOptions Options, FEOSRTCAudioInputDeviceInformation& OutInputDeviceInformation)
{
	LogEOSVerbose("");

	EOSResult Result = EOSResult::EOS_UnexpectedError;

	if (Handle)
	{
		EOS_RTCAudio_CopyInputDeviceInformationByIndexOptions Parameters = {};
		Parameters.ApiVersion = EOS_RTCAUDIO_COPYINPUTDEVICEINFORMATIONBYINDEX_API_LATEST;
		Parameters.DeviceIndex = Options.DeviceIndex;

		EOS_RTCAudio_InputDeviceInformation* InputDeviceInformation = NULL;

		Result = EOSHelpers::ToEOSCoreResult(EOS_RTCAudio_CopyInputDeviceInformationByIndex(Handle, &Parameters, &InputDeviceInformation));

		if (Result == EOSResult::EOS_Success)
		{
			OutInputDeviceInformation.bDefaultDevice = (InputDeviceInformation->bDefaultDevice > 0);
			const FUTF8ToTCHAR DeviceId(InputDeviceInformation->DeviceId);
			const FUTF8ToTCHAR DeviceName(InputDeviceInformation->DeviceName);
			OutInputDeviceInformation.DeviceId = DeviceId.Get();
			OutInputDeviceInformation.DeviceName = DeviceName.Get();

			EOS_RTCAudio_InputDeviceInformation_Release(InputDeviceInformation);
		}
	}

	return Result;
}

void UCoreRTCAudio::EOSRTCAudioQueryOutputDevicesInformation(FEOSHRTCAudio Handle, const FOnQueryOutputDevicesInformationCallback& Callback)
{
	LogEOSVerbose("");

	if (Handle)
	{
		EOS_RTCAudio_QueryOutputDevicesInformationOptions Parameters = {};
		Parameters.ApiVersion = EOS_RTCAUDIO_QUERYOUTPUTDEVICESINFORMATION_API_LATEST;

		FQueryOutputDevicesInformationCallback* CallbackObj = new FQueryOutputDevicesInformationCallback(Callback);
		EOS_RTCAudio_QueryOutputDevicesInformation(Handle, &Parameters, CallbackObj, Internal_QueryOutputDevicesInformationCallback);
	}
}

int32 UCoreRTCAudio::EOSRTCAudioGetOutputDevicesCount(FEOSHRTCAudio Handle)
{
	LogEOSVerbose("");

	int32 Result = 0;

	if (Handle)
	{
		EOS_RTCAudio_GetOutputDevicesCountOptions Parameters = {};
		Parameters.ApiVersion = EOS_RTCAUDIO_GETOUTPUTDEVICESCOUNT_API_LATEST;

		Result = EOS_RTCAudio_GetOutputDevicesCount(Handle, &Parameters);
	}

	return Result;
}

EOSResult UCoreRTCAudio::EOSRTCAudioCopyOutputDeviceInformationByIndex(FEOSHRTCAudio Handle, FEOSRTCAudioCopyOutputDeviceInformationByIndexOptions Options, FEOSRTCAudioOutputDeviceInformation& OutOutputDeviceInformation)
{
	LogEOSVerbose("");

	EOSResult Result = EOSResult::EOS_UnexpectedError;

	if (Handle)
	{
		EOS_RTCAudio_CopyOutputDeviceInformationByIndexOptions Parameters = {};
		Parameters.ApiVersion = EOS_RTCAUDIO_COPYINPUTDEVICEINFORMATIONBYINDEX_API_LATEST;
		Parameters.DeviceIndex = Options.DeviceIndex;

		EOS_RTCAudio_OutputDeviceInformation* OutputDeviceInformation = NULL;

		Result = EOSHelpers::ToEOSCoreResult(EOS_RTCAudio_CopyOutputDeviceInformationByIndex(Handle, &Parameters, &OutputDeviceInformation));

		if (Result == EOSResult::EOS_Success)
		{
			OutOutputDeviceInformation.bDefaultDevice = (OutputDeviceInformation->bDefaultDevice > 0);
			const FUTF8ToTCHAR DeviceId(OutputDeviceInformation->DeviceId);
			const FUTF8ToTCHAR DeviceName(OutputDeviceInformation->DeviceName);
			OutOutputDeviceInformation.DeviceId = DeviceId.Get();
			OutOutputDeviceInformation.DeviceName = DeviceName.Get();

			EOS_RTCAudio_OutputDeviceInformation_Release(OutputDeviceInformation);
		}
	}

	return Result;
}

void UCoreRTCAudio::EOSRTCAudioSetInputDeviceSettings(FEOSHRTCAudio Handle, FEOSRTCAudioSetInputDeviceSettingsOptions Options, const FOnSetInputDeviceSettingsCallback& Callback)
{
	LogEOSVerbose("");

	if (Handle)
	{
		EOS_RTCAudio_SetInputDeviceSettingsOptions Parameters = {};
		Parameters.ApiVersion = EOS_RTCAUDIO_SETINPUTDEVICESETTINGS_API_LATEST;
		Parameters.LocalUserId = Options.LocalUserId;
		const FTCHARToUTF8 RealDeviceId(*Options.RealDeviceId);
		Parameters.RealDeviceId = RealDeviceId.Get();
		Parameters.bPlatformAEC = Options.bPlatformAEC;

		FSetInputDeviceSettingsCallback* CallbackObj = new FSetInputDeviceSettingsCallback(Callback);
		EOS_RTCAudio_SetInputDeviceSettings(Handle, &Parameters, CallbackObj, Internal_SetInputDeviceSettingsCallback);
	}
}

void UCoreRTCAudio::EOSRTCAudioSetOutputDeviceSettings(FEOSHRTCAudio Handle, FEOSRTCAudioSetOutputDeviceSettingsOptions Options, const FOnSetOutputDeviceSettingsCallback& Callback)
{
	LogEOSVerbose("");

	if (Handle)
	{
		EOS_RTCAudio_SetOutputDeviceSettingsOptions Parameters = {};
		Parameters.ApiVersion = EOS_RTCAUDIO_SETINPUTDEVICESETTINGS_API_LATEST;
		Parameters.LocalUserId = Options.LocalUserId;
		const FTCHARToUTF8 RealDeviceId(*Options.RealDeviceId);
		Parameters.RealDeviceId = RealDeviceId.Get();

		FSetOutputDeviceSettingsCallback* CallbackObj = new FSetOutputDeviceSettingsCallback(Callback);
		EOS_RTCAudio_SetOutputDeviceSettings(Handle, &Parameters, CallbackObj, Internal_SetOutputDeviceSettingsCallback);
	}
}

void UCoreRTCAudio::Internal_OnUpdateSendingCallback(const EOS_RTCAudio_UpdateSendingCallbackInfo* Data)
{
	LogEOSVerbose("%s", *FString(EOS_EResult_ToString(Data->ResultCode)));

	const FUpdateSendingOptionsCallback* CallbackObj = static_cast<FUpdateSendingOptionsCallback*>(Data->ClientData);
	check(CallbackObj);
	if (CallbackObj)
	{
		CallbackObj->m_Callback.ExecuteIfBound(*Data);
		delete CallbackObj;
	}
}

void UCoreRTCAudio::Internal_OnUpdateReceivingCallback(const EOS_RTCAudio_UpdateReceivingCallbackInfo* Data)
{
	LogEOSVerbose("%s", *FString(EOS_EResult_ToString(Data->ResultCode)));

	const FUpdateReceivingOptionsCallback* CallbackObj = static_cast<FUpdateReceivingOptionsCallback*>(Data->ClientData);
	check(CallbackObj);
	if (CallbackObj)
	{
		CallbackObj->m_Callback.ExecuteIfBound(*Data);
		delete CallbackObj;
	}
}

void UCoreRTCAudio::Internal_UpdateSendingVolumeCallback(const EOS_RTCAudio_UpdateSendingVolumeCallbackInfo* Data)
{
	LogEOSVerbose("%s", *FString(EOS_EResult_ToString(Data->ResultCode)));

	const FUpdateSendingVolumeCallback* CallbackObj = static_cast<FUpdateSendingVolumeCallback*>(Data->ClientData);
	check(CallbackObj);
	if (CallbackObj)
	{
		CallbackObj->m_Callback.ExecuteIfBound(*Data);
		delete CallbackObj;
	}
}

void UCoreRTCAudio::Internal_UpdateReceivingVolumeCallback(const EOS_RTCAudio_UpdateReceivingVolumeCallbackInfo* Data)
{
	LogEOSVerbose("%s", *FString(EOS_EResult_ToString(Data->ResultCode)));

	const FUpdateReceivingVolumeCallback* CallbackObj = static_cast<FUpdateReceivingVolumeCallback*>(Data->ClientData);
	check(CallbackObj);
	if (CallbackObj)
	{
		CallbackObj->m_Callback.ExecuteIfBound(*Data);
		delete CallbackObj;
	}
}

void UCoreRTCAudio::Internal_UpdateParticipantVolumeCallback(const EOS_RTCAudio_UpdateParticipantVolumeCallbackInfo* Data)
{
	LogEOSVerbose("%s", *FString(EOS_EResult_ToString(Data->ResultCode)));

	const FUpdateParticipantVolumeCallback* CallbackObj = static_cast<FUpdateParticipantVolumeCallback*>(Data->ClientData);
	check(CallbackObj);
	if (CallbackObj)
	{
		CallbackObj->m_Callback.ExecuteIfBound(*Data);
		delete CallbackObj;
	}
}

void UCoreRTCAudio::Internal_RegisterPlatformUserCallback(const EOS_RTCAudio_OnRegisterPlatformUserCallbackInfo* Data)
{
	LogEOSVerbose("%s", *FString(EOS_EResult_ToString(Data->ResultCode)));

	const FRegisterPlatformUserCallback* CallbackObj = static_cast<FRegisterPlatformUserCallback*>(Data->ClientData);
	check(CallbackObj);
	if (CallbackObj)
	{
		CallbackObj->m_Callback.ExecuteIfBound(*Data);
		delete CallbackObj;
	}
}

void UCoreRTCAudio::Internal_UnregisterPlatformUserCallback(const EOS_RTCAudio_OnUnregisterPlatformUserCallbackInfo* Data)
{
	LogEOSVerbose("%s", *FString(EOS_EResult_ToString(Data->ResultCode)));

	const FUnregisterPlatformUserCallback* CallbackObj = static_cast<FUnregisterPlatformUserCallback*>(Data->ClientData);
	check(CallbackObj);
	if (CallbackObj)
	{
		CallbackObj->m_Callback.ExecuteIfBound(*Data);
		delete CallbackObj;
	}
}

void UCoreRTCAudio::Internal_QueryInputDevicesInformationCallback(const EOS_RTCAudio_OnQueryInputDevicesInformationCallbackInfo* Data)
{
	LogEOSVerbose("%s", *FString(EOS_EResult_ToString(Data->ResultCode)));

	const FQueryInputDevicesInformationCallback* CallbackObj = static_cast<FQueryInputDevicesInformationCallback*>(Data->ClientData);
	check(CallbackObj);
	if (CallbackObj)
	{
		CallbackObj->m_Callback.ExecuteIfBound(*Data);
		delete CallbackObj;
	}
}

void UCoreRTCAudio::Internal_QueryOutputDevicesInformationCallback(const EOS_RTCAudio_OnQueryOutputDevicesInformationCallbackInfo* Data)
{
	LogEOSVerbose("%s", *FString(EOS_EResult_ToString(Data->ResultCode)));

	const FQueryOutputDevicesInformationCallback* CallbackObj = static_cast<FQueryOutputDevicesInformationCallback*>(Data->ClientData);
	check(CallbackObj);
	if (CallbackObj)
	{
		CallbackObj->m_Callback.ExecuteIfBound(*Data);
		delete CallbackObj;
	}
}

void UCoreRTCAudio::Internal_SetInputDeviceSettingsCallback(const EOS_RTCAudio_OnSetInputDeviceSettingsCallbackInfo* Data)
{
	LogEOSVerbose("%s", *FString(EOS_EResult_ToString(Data->ResultCode)));

	const FSetInputDeviceSettingsCallback* CallbackObj = static_cast<FSetInputDeviceSettingsCallback*>(Data->ClientData);
	check(CallbackObj);
	if (CallbackObj)
	{
		CallbackObj->m_Callback.ExecuteIfBound(*Data);
		delete CallbackObj;
	}
}

void UCoreRTCAudio::Internal_SetOutputDeviceSettingsCallback(const EOS_RTCAudio_OnSetOutputDeviceSettingsCallbackInfo* Data)
{
	LogEOSVerbose("%s", *FString(EOS_EResult_ToString(Data->ResultCode)));

	const FSetOutputDeviceSettingsCallback* CallbackObj = static_cast<FSetOutputDeviceSettingsCallback*>(Data->ClientData);
	check(CallbackObj);
	if (CallbackObj)
	{
		CallbackObj->m_Callback.ExecuteIfBound(*Data);
		delete CallbackObj;
	}
}
