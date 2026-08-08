/**
* Copyright (C) 2017-2025 | eelDev AB
*
* EOSCore Documentation: https://eeldev.com
*/

#include "EOSWebConnect.h"
#include "EOSCoreWebModule.h"
#include "EOSCoreWebLogging.h"
#include "EOSCoreWebPrivatePCH.h"
#include "Shared/EOSWebShared.h"

void UEOSWebConnectLibrary::RequestAccessToken(FRequestAccessTokenRequest Request, const FAccessTokenCallbackDelegate& Callback)
{
	LogEOSVerbose("");

	if (Request.Credentials.ClientId.IsEmpty())
	{
		LogEOSError("ClientId cannot be empty");
		return;
	}

	if (Request.Credentials.ClientSecret.IsEmpty())
	{
		LogEOSError("ClientSecret cannot be empty");
		return;
	}

	if (Request.DeploymentId.IsEmpty())
	{
		LogEOSError("DeploymentId cannot be empty");
		return;
	}

	auto HttpRequest = UEOSWebShared::CreateRequest();

	const FString Base64Credentials = FBase64::Encode(FString::Printf(TEXT("%s:%s"), *Request.Credentials.ClientId, *Request.Credentials.ClientSecret));
	const FString ContentString = FString::Printf(TEXT("grant_type=client_credentials&deployment_id=%s&nonce=%s"), *Request.DeploymentId, *Request.AdditionalData);

	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/x-www-form-urlencoded"));
	HttpRequest->AppendToHeader(TEXT("Accept"), TEXT("application/json"));
	HttpRequest->AppendToHeader(TEXT("Authorization"), *FString::Printf(TEXT("Basic %s"), *Base64Credentials));
	HttpRequest->SetContentAsString(ContentString);
	HttpRequest->SetURL("https://api.epicgames.dev/auth/v1/oauth/token");
	HttpRequest->SetVerb("POST");

	HttpRequest->OnProcessRequestComplete().BindLambda([=](FHttpRequestPtr HttpRequestPtr, FHttpResponsePtr HttpResponsePtr, bool bConnectedSuccessfully)
	{
		FString AccessTokenString;
		FString TokenTypeString;
		FString ExpiresAtString;
		FString ExpiresInString;
		FString NonceString;
		TArray<FString> FeaturesString;
		FString OrganizationString;
		FString ProductIdString;
		FString SandboxIdString;
		FString DeploymentIdString;

		if (bConnectedSuccessfully)
		{
			LogEOSVeryVerbose("RequestAccessToken Response: %s", *HttpResponsePtr->GetContentAsString());

			TSharedPtr<FJsonObject> JsonObject;
			TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(HttpResponsePtr->GetContentAsString());

			if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
			{
				TSharedPtr<FJsonValue> AccessTokenObject = JsonObject->TryGetField(TEXT("access_token"));
				TSharedPtr<FJsonValue> TokenTypeObject = JsonObject->TryGetField(TEXT("token_type"));
				TSharedPtr<FJsonValue> ExpiresAtObject = JsonObject->TryGetField(TEXT("expires_at"));
				TSharedPtr<FJsonValue> ExpiresInObject = JsonObject->TryGetField(TEXT("expires_in"));
				TSharedPtr<FJsonValue> NonceObject = JsonObject->TryGetField(TEXT("nonce"));
				TSharedPtr<FJsonValue> FeaturesObject = JsonObject->TryGetField(TEXT("features"));
				TSharedPtr<FJsonValue> OrganizationIdObject = JsonObject->TryGetField(TEXT("organization_id"));
				TSharedPtr<FJsonValue> ProductIdObject = JsonObject->TryGetField(TEXT("product_id"));
				TSharedPtr<FJsonValue> SandboxIdObject = JsonObject->TryGetField(TEXT("sandbox_id"));
				TSharedPtr<FJsonValue> DeploymentIdObject = JsonObject->TryGetField(TEXT("deployment_id"));

				if (AccessTokenObject)
				{
					AccessTokenString = AccessTokenObject->AsString();
				}
				if (TokenTypeObject)
				{
					TokenTypeString = TokenTypeObject->AsString();
				}
				if (ExpiresAtObject)
				{
					ExpiresAtString = ExpiresAtObject->AsString();
				}
				if (ExpiresInObject)
				{
					ExpiresInString = ExpiresInObject->AsString();
				}
				if (NonceObject)
				{
					NonceString = NonceObject->AsString();
				}
				if (FeaturesObject)
				{
					if (FeaturesObject->Type == EJson::Array)
					{
						auto& JsonArray = FeaturesObject->AsArray();
						for (auto& Element : JsonArray)
						{
							FeaturesString.Add(Element->AsString());
						}
					}
				}
				if (OrganizationIdObject)
				{
					OrganizationString = OrganizationIdObject->AsString();
				}
				if (ProductIdObject)
				{
					ProductIdString = ProductIdObject->AsString();
				}
				if (SandboxIdObject)
				{
					SandboxIdString = SandboxIdObject->AsString();
				}
				if (DeploymentIdObject)
				{
					DeploymentIdString = DeploymentIdObject->AsString();
				}
			}
		}
		else
		{
			LogEOSError("Connection failed");
		}

		if (bConnectedSuccessfully && !AccessTokenString.IsEmpty())
		{
			Callback.ExecuteIfBound(true, FAccessTokenCallbackData(AccessTokenString, TokenTypeString, ExpiresAtString, ExpiresInString, NonceString, FeaturesString, OrganizationString, ProductIdString, SandboxIdString, DeploymentIdString), FWebResponse(HttpResponsePtr->GetResponseCode(), HttpResponsePtr->GetContentAsString()));
		}
		else
		{
			Callback.ExecuteIfBound(false, FAccessTokenCallbackData(), FWebResponse(HttpResponsePtr->GetResponseCode(), HttpResponsePtr->GetContentAsString()));
		}
	});

	LogEOSVerbose("-------------------------------------------------\n");
	LogEOSVerbose("Sending Request AccessToken (%s)", *HttpRequest->GetURL());
	for (auto& Element : HttpRequest->GetAllHeaders())
	{
		LogEOSVerbose("Header: %s", *Element);
	}
	LogEOSVerbose("Content: %s", *ContentString);
	LogEOSVerbose("-------------------------------------------------\n");

	HttpRequest->ProcessRequest();
}