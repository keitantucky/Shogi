/**
* Copyright (C) 2017-2025 | eelDev AB
*
* EOSCore Documentation: https://eeldev.com
*/

#include "EOSWebVoice.h"
#include "EOSCoreWebModule.h"
#include "EOSCoreWebLogging.h"
#include "EOSCoreWebPrivatePCH.h"
#include "Shared/EOSWebShared.h"

struct FWebRequestData : public FJsonSerializable
{
	struct FRequestUser : public FJsonSerializable
	{
		explicit FRequestUser() = default;
		FString ProductUserId;
		FString ClientIp;
		bool bHardMuted;

		BEGIN_JSON_SERIALIZER
			JSON_SERIALIZE("puid", ProductUserId);
		JSON_SERIALIZE("clientIp", ClientIp);
		JSON_SERIALIZE("hardMuted", bHardMuted);
		END_JSON_SERIALIZER
};

	FWebRequestData()
	{
	}

	FWebRequestData(TArray<FWebRequestParticipantData> Users)
	{
		for (auto& Element : Users)
		{
			FRequestUser User;
			User.ProductUserId = Element.ProductUserId;
			User.ClientIp = Element.ClientIp;
			User.bHardMuted = Element.bHardMuted;

			UsersArray.Add(User);
		}
	}

	TArray<FRequestUser> UsersArray;

	BEGIN_JSON_SERIALIZER
		JSON_SERIALIZE_ARRAY_SERIALIZABLE("participants", UsersArray, FRequestUser);
	END_JSON_SERIALIZER
};

void UEOSWebVoiceLibrary::CreateRoomToken(FString AccessToken, TArray<FWebRequestParticipantData> Participants, FString DeploymentId, FString RoomName, const FCreateRoomTokenCallbackDelegate& Callback)
{
	LogEOSVerbose("");

	if (AccessToken.IsEmpty())
	{
		LogEOSError("AccessToken cannot be empty");
		return;
	}

	if (DeploymentId.IsEmpty())
	{
		LogEOSError("DeploymentId cannot be empty");
		return;
	}

	if (Participants.Num() == 0)
	{
		LogEOSError("Participants cannot be empty");
		return;
	}

	if (RoomName.IsEmpty())
	{
		LogEOSError("RoomName cannot be empty");
		return;
	}

	auto HttpRequest = UEOSWebShared::CreateRequest();

	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->AppendToHeader(TEXT("Accept"), TEXT("application/json"));
	HttpRequest->AppendToHeader(TEXT("Authorization"), *FString::Printf(TEXT("Bearer %s"), *AccessToken));

	FWebRequestData RequestData(Participants);
	const FString ContentString = *RequestData.ToJson(false);

	HttpRequest->SetContentAsString(ContentString);
	HttpRequest->SetURL(FString::Printf(TEXT("https://api.epicgames.dev/rtc/v1/%s/room/%s"), *DeploymentId, *RoomName));
	HttpRequest->SetVerb("POST");
	HttpRequest->OnProcessRequestComplete().BindLambda([=](FHttpRequestPtr HttpRequestPtr, FHttpResponsePtr HttpResponsePtr, bool bConnectedSuccessfully)
	{
		FString RoomIdString;
		FString DeploymentIdString;
		FString ClientBaseUrlString;
		FString PuidString;
		FString TokenString;

		if (bConnectedSuccessfully)
		{
			LogEOSVeryVerbose("CreateRoomToken Response: %s", *HttpResponsePtr->GetContentAsString());

			if (HttpResponsePtr->GetResponseCode() == 200)
			{
				TSharedPtr<FJsonObject> JsonObject;
				const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(HttpResponsePtr->GetContentAsString());

				if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
				{
					const TSharedPtr<FJsonValue> RoomIdObject = JsonObject->TryGetField(TEXT("roomId"));
					const TSharedPtr<FJsonValue> DeploymentIdObject = JsonObject->TryGetField(TEXT("deploymentId"));
					const TSharedPtr<FJsonValue> ClientBaseUrlObject = JsonObject->TryGetField(TEXT("clientBaseUrl"));
					const TSharedPtr<FJsonValue> ParticipantsObject = JsonObject->TryGetField(TEXT("participants"));

					if (!RoomIdObject)
					{
						LogEOSError("No RoomId was found");
					}

					if (!DeploymentIdObject)
					{
						LogEOSError("No DeploymentId was found");
					}

					if (!ClientBaseUrlObject)
					{
						LogEOSError("No ClientBaseUrl was found");
					}

					if (!ParticipantsObject)
					{
						LogEOSError("No Participants was found");
					}

					if (RoomIdObject && DeploymentIdObject && ClientBaseUrlObject && ParticipantsObject)
					{
						RoomIdString = RoomIdObject->AsString();
						DeploymentIdString = DeploymentIdObject->AsString();
						ClientBaseUrlString = ClientBaseUrlObject->AsString();

						if (ParticipantsObject && ParticipantsObject->Type == EJson::Array)
						{
							TArray<TSharedPtr<FJsonValue>> ParticipantsArray = ParticipantsObject->AsArray();

							for (const auto& Element : ParticipantsArray)
							{
								if (Element->Type != EJson::Object)
									continue;

								auto& Object = Element->AsObject();

								TokenString = Object->TryGetField("token")->AsString();
								PuidString = Object->TryGetField("puid")->AsString();
							}
						}
					}
				}
			}
			else
			{
				LogEOSError("Error Code: %d", HttpResponsePtr->GetResponseCode());
			}
		}
		else
		{
			LogEOSError("Connection failed");
		}

		if (!RoomIdString.IsEmpty() && !DeploymentIdString.IsEmpty() && !ClientBaseUrlString.IsEmpty() && !TokenString.IsEmpty() && !PuidString.IsEmpty())
		{
			Callback.ExecuteIfBound(true, FCreateRoomTokenCallback(RoomIdString, DeploymentIdString, ClientBaseUrlString, TokenString, PuidString), FWebResponse(HttpResponsePtr->GetResponseCode(), HttpResponsePtr->GetContentAsString()));
		}
		else
		{
			Callback.ExecuteIfBound(false, FCreateRoomTokenCallback(RoomIdString, DeploymentIdString, ClientBaseUrlString, TokenString, PuidString), FWebResponse(HttpResponsePtr->GetResponseCode(), HttpResponsePtr->GetContentAsString()));
		}
	});

	LogEOSVerbose("-------------------------------------------------\n");
	LogEOSVerbose("Sending CreateRoom Request (%s)", *HttpRequest->GetURL());
	for (auto& Element : HttpRequest->GetAllHeaders())
	{
		LogEOSVerbose("Header: %s", *Element);
	}
	LogEOSVerbose("Content: %s", *ContentString);
	LogEOSVerbose("-------------------------------------------------\n");

	HttpRequest->ProcessRequest();
}

void UEOSWebVoiceLibrary::RemoveParticipant(FString AccessToken, FString ProductUserId, FString DeploymentId, FString RoomName, const FRemoveParticipantCallbackDelegate& Callback)
{
	LogEOSVerbose("");

	if (AccessToken.IsEmpty())
	{
		LogEOSError("AccessToken cannot be empty");
		return;
	}

	if (ProductUserId.IsEmpty())
	{
		LogEOSError("ProductUserId cannot be empty");
		return;
	}

	if (DeploymentId.IsEmpty())
	{
		LogEOSError("DeploymentId cannot be empty");
		return;
	}

	if (RoomName.IsEmpty())
	{
		LogEOSError("RoomName cannot be empty");
		return;
	}

	auto HttpRequest = UEOSWebShared::CreateRequest();

	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->AppendToHeader(TEXT("Accept"), TEXT("application/json"));
	HttpRequest->AppendToHeader(TEXT("Authorization"), *FString::Printf(TEXT("Bearer %s"), *AccessToken));
	HttpRequest->SetURL(FString::Printf(TEXT("https://api.epicgames.dev/rtc/v1/%s/room/%s/participants/%s"), *DeploymentId, *RoomName, *ProductUserId));
	HttpRequest->SetVerb("DELETE");
	HttpRequest->OnProcessRequestComplete().BindLambda([=](FHttpRequestPtr HttpRequestPtr, FHttpResponsePtr HttpResponsePtr, bool bConnectedSuccessfully)
	{
		if (bConnectedSuccessfully)
		{
			LogEOSVeryVerbose("RemoveParticipant Response: (%d) (%s)", HttpResponsePtr->GetResponseCode(), *HttpResponsePtr->GetContentAsString());
		}
		else
		{
			LogEOSError("Connection failed");
		}

		Callback.ExecuteIfBound(HttpResponsePtr->GetResponseCode() == 204, FWebResponse(HttpResponsePtr->GetResponseCode(), HttpResponsePtr->GetContentAsString()));
	});

	LogEOSVerbose("-------------------------------------------------\n");
	LogEOSVerbose("Sending RemoveParticipant Request (%s)", *HttpRequest->GetURL());
	for (auto& Element : HttpRequest->GetAllHeaders())
	{
		LogEOSVerbose("Header: %s", *Element);
	}
	LogEOSVerbose("-------------------------------------------------\n");

	HttpRequest->ProcessRequest();
}

void UEOSWebVoiceLibrary::ModifyParticipant(FString AccessToken, FString ProductUserId, FString DeploymentId, FString RoomName, bool bHardMuted, const FModifyParticipantCallbackDelegate& Callback)
{
	LogEOSVerbose("");

	if (AccessToken.IsEmpty())
	{
		LogEOSError("AccessToken cannot be empty");
		return;
	}

	if (ProductUserId.IsEmpty())
	{
		LogEOSError("ProductUserId cannot be empty");
		return;
	}

	if (DeploymentId.IsEmpty())
	{
		LogEOSError("DeploymentId cannot be empty");
		return;
	}

	if (RoomName.IsEmpty())
	{
		LogEOSError("RoomName cannot be empty");
		return;
	}

	auto HttpRequest = UEOSWebShared::CreateRequest();

	const FString ContentString = FString::Printf(TEXT("{\"hardMuted\":%s}"), *LexToString(bHardMuted));

	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->AppendToHeader(TEXT("Accept"), TEXT("application/json"));
	HttpRequest->AppendToHeader(TEXT("Authorization"), *FString::Printf(TEXT("Bearer %s"), *AccessToken));
	HttpRequest->SetContentAsString(ContentString);
	HttpRequest->SetURL(FString::Printf(TEXT("https://api.epicgames.dev/rtc/v1/%s/room/%s/participants/%s"), *DeploymentId, *RoomName, *ProductUserId));
	HttpRequest->SetVerb("POST");

	HttpRequest->OnProcessRequestComplete().BindLambda([=](FHttpRequestPtr HttpRequestPtr, FHttpResponsePtr HttpResponsePtr, bool bConnectedSuccessfully)
	{
		if (bConnectedSuccessfully)
		{
			LogEOSVeryVerbose("ModifyParticipant Response: (%d) (%s)", HttpResponsePtr->GetResponseCode(), *HttpResponsePtr->GetContentAsString());
		}
		else
		{
			LogEOSError("Connection failed");
		}

		Callback.ExecuteIfBound(HttpResponsePtr->GetResponseCode() == 204, FWebResponse(HttpResponsePtr->GetResponseCode(), HttpResponsePtr->GetContentAsString()));
	});

	LogEOSVerbose("-------------------------------------------------\n");
	LogEOSVerbose("Sending ModifyParticipant Request (%s)", *HttpRequest->GetURL());
	for (auto& Element : HttpRequest->GetAllHeaders())
	{
		LogEOSVerbose("Header: %s", *Element);
	}
	LogEOSVerbose("Content: %s", *ContentString);
	LogEOSVerbose("-------------------------------------------------\n");

	HttpRequest->ProcessRequest();
}