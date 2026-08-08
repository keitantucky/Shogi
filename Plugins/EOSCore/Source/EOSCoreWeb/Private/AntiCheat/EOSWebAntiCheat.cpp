/**
* Copyright (C) 2017-2025 | eelDev AB
*
* EOSCore Documentation: https://eeldev.com
*/

#include "EOSWebAntiCheat.h"
#include "EOSCoreWebModule.h"
#include "EOSCoreWebLogging.h"
#include "EOSCoreWebPrivatePCH.h"
#include "Shared/EOSWebShared.h"

void UEOSWebAntiCheatLibrary::QueryAntiCheatServiceStatusByDeployment(FString AccessToken, FString DeploymentId, const FAntiCheatStatusCallbackDelegate& Callback)
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

	auto HttpRequest = UEOSWebShared::CreateRequest();

	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->AppendToHeader(TEXT("Accept"), TEXT("application/json"));
	HttpRequest->AppendToHeader(TEXT("Authorization"), *FString::Printf(TEXT("Bearer %s"), *AccessToken));
	HttpRequest->SetURL(FString::Printf(TEXT("https://api.epicgames.dev/anticheat/v1/%s/status"), *DeploymentId));
	HttpRequest->SetVerb("GET");

	HttpRequest->OnProcessRequestComplete().BindLambda([=](FHttpRequestPtr HttpRequestPtr, FHttpResponsePtr HttpResponsePtr, bool bConnectedSuccessfully)
	{
		bool bServerKick = false;
		
		if (bConnectedSuccessfully)
		{
			LogEOSVerbose("QueryAntiCheatServiceStatusByDeployment Response: %s", *HttpResponsePtr->GetContentAsString());

			if (HttpResponsePtr->GetResponseCode() == 200)
			{
				TSharedPtr<FJsonObject> JsonObject;
				const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(HttpResponsePtr->GetContentAsString());

				if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
				{
					const TSharedPtr<FJsonValue> ServerKickObject = JsonObject->TryGetField(TEXT("serverKick"));

					if (!ServerKickObject)
					{
						LogEOSError("serverKick was not found");
					}

					if (ServerKickObject)
					{
						bServerKick = ServerKickObject->AsBool();
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

		Callback.ExecuteIfBound(HttpResponsePtr->GetResponseCode() == 200, FQueryAntiCheatServiceStatusByDeploymentCallbackData(bServerKick), FWebResponse(HttpResponsePtr->GetResponseCode(), HttpResponsePtr->GetContentAsString()));
	});

	LogEOSVerbose("-------------------------------------------------\n");
	LogEOSVerbose("Sending QueryAntiCheatServiceStatusByDeployment Request (%s)", *HttpRequest->GetURL());
	for (auto& Element : HttpRequest->GetAllHeaders())
	{
		LogEOSVerbose("Header: %s", *Element);
	}
	LogEOSVerbose("-------------------------------------------------\n");

	HttpRequest->ProcessRequest();
}
