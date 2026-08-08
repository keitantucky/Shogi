/**
* Copyright (C) 2017-2025 | eelDev AB
*
* EOSCore Documentation: https://eeldev.com
*/

#include "EOSWebPlayerTicket.h"
#include "EOSCoreWebModule.h"
#include "EOSCoreWebLogging.h"
#include "EOSCoreWebPrivatePCH.h"
#include "Shared/EOSWebShared.h"

struct FSubmitTicketJson final : public FJsonSerializable
{
	explicit FSubmitTicketJson(FSubmitTicketRequest Request)
		: Message(Request.Message)
		, SenderEmail(Request.SenderEmail)
		, SenderName(Request.SenderName)
		, GUID(Request.GUID)
		, ErrorCode(Request.ErrorCode)
		, SystemOS(Request.SystemOS)
		, SystemAntiMalware(Request.SystemAntiMalware)
		, SystemOther(Request.SystemOther)
	{
		switch (Request.Subject)
		{
		case ESubjectType::BAN_APPEAL:
			Subject = "ban-appeal";
			break;
		case ESubjectType::CHEAT_REPORT:
			Subject = "cheat-report";
			break;
		case ESubjectType::OPEN_QUESTION:
			Subject = "open-question";
			break;
		case ESubjectType::TECHNICAL_SUPPORT:
			Subject = "technical-support";
			break;
		default: ;
		}
	}

	FString Subject;
	FString Message;
	FString SenderEmail;
	FString SenderName;
	FString GUID;
	FString ErrorCode;
	FString SystemOS;
	FString SystemAntiMalware;
	FString SystemOther;

	BEGIN_JSON_SERIALIZER
		JSON_SERIALIZE("subject", Subject);
		JSON_SERIALIZE("message", Message);
		JSON_SERIALIZE("sender_email", SenderEmail);
		JSON_SERIALIZE("sender_name", SenderName);
		JSON_SERIALIZE("guid", GUID);
		JSON_SERIALIZE("error_code", ErrorCode);
		JSON_SERIALIZE("system_os", SystemOS);
		JSON_SERIALIZE("system_antimalware", SystemAntiMalware);
		JSON_SERIALIZE("system_other", SystemOther);
		END_JSON_SERIALIZER
};

void UEOSWebPlayerTicketLibrary::SubmitTicket(FString WebApiKey, FSubmitTicketRequest Request, const FSubmitTicketCallbackDelegate& Callback)
{
	LogEOSVerbose("");

	if (Request.Subject == ESubjectType::NOT_SET)
	{
		LogEOSError("Subject must be set");
		return;
	}

	if (Request.Message.IsEmpty())
	{
		LogEOSError("Message cannot be empty");
		return;
	}

	if (Request.SenderEmail.IsEmpty())
	{
		LogEOSError("SenderEmail cannot be empty");
		return;
	}

	auto HttpRequest = UEOSWebShared::CreateRequest();

	FSubmitTicketJson TicketJson(Request);
	const FString ContentString = TicketJson.ToJson(false);

	HttpRequest->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
	HttpRequest->AppendToHeader(TEXT("Authorization"), *FString::Printf(TEXT("Token %s"), *WebApiKey));
	HttpRequest->SetContentAsString(ContentString);
	HttpRequest->SetURL("https://dev.epicgames.com/portal/api/v1/services/tickets/submit/");
	HttpRequest->SetVerb("POST");

	HttpRequest->OnProcessRequestComplete().BindLambda([=](FHttpRequestPtr HttpRequestPtr, FHttpResponsePtr HttpResponsePtr, bool bConnectedSuccessfully)
	{
		if (bConnectedSuccessfully)
		{
			LogEOSVeryVerbose("SubmitTicket Response: (%d) (%s)", HttpResponsePtr->GetResponseCode(), *HttpResponsePtr->GetContentAsString());
		}
		else
		{
			LogEOSError("Connection failed");
		}

		TSharedPtr<FJsonObject> JsonObject;
		const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(HttpResponsePtr->GetContentAsString());

		FSubmitTicketResponse OutData = {};

		if (FJsonSerializer::Deserialize(JsonReader, JsonObject) && JsonObject.IsValid())
		{
			const TSharedPtr<FJsonValue> DataObject = JsonObject->TryGetField(TEXT("data"));

			if (DataObject)
			{
				const TSharedPtr<FJsonValue> SubjectObject = DataObject->AsObject()->TryGetField(TEXT("subject"));
				const TSharedPtr<FJsonValue> MessageObject = DataObject->AsObject()->TryGetField(TEXT("message"));
				const TSharedPtr<FJsonValue> SenderEmailObject = DataObject->AsObject()->TryGetField(TEXT("sender_email"));
				const TSharedPtr<FJsonValue> SenderNameObject = DataObject->AsObject()->TryGetField(TEXT("sender_name"));
				const TSharedPtr<FJsonValue> GuidObject = DataObject->AsObject()->TryGetField(TEXT("guid"));
				const TSharedPtr<FJsonValue> ErrorCodeObject = DataObject->AsObject()->TryGetField(TEXT("error_code"));
				const TSharedPtr<FJsonValue> SystemOsObject = DataObject->AsObject()->TryGetField(TEXT("system_os"));
				const TSharedPtr<FJsonValue> SystemAntiMalwareObject = DataObject->AsObject()->TryGetField(TEXT("system_antimalware"));
				const TSharedPtr<FJsonValue> SystemOtherObject = DataObject->AsObject()->TryGetField(TEXT("system_other"));
				const TSharedPtr<FJsonValue> TimestampObject = DataObject->AsObject()->TryGetField(TEXT("timestamp"));

				if (SubjectObject)
				{
					OutData.Subject = SubjectObject->AsString();
				}
				if (MessageObject)
				{
					OutData.Message = MessageObject->AsString();
				}
				if (SenderEmailObject)
				{
					OutData.SenderEmail = SenderEmailObject->AsString();
				}
				if (SenderNameObject)
				{
					OutData.SenderName = SenderNameObject->AsString();
				}
				if (GuidObject)
				{
					OutData.GUID = GuidObject->AsString();
				}
				if (ErrorCodeObject)
				{
					OutData.ErrorCode = ErrorCodeObject->AsString();
				}
				if (SystemOsObject)
				{
					OutData.SystemOS = SystemOsObject->AsString();
				}
				if (SystemAntiMalwareObject)
				{
					OutData.SystemAntiMalware = SystemAntiMalwareObject->AsString();
				}
				if (SystemOtherObject)
				{
					OutData.SystemOther = SystemOtherObject->AsString();
				}
				if (TimestampObject)
				{
					OutData.Timestamp = TimestampObject->AsString();
				}
			}
		}

		Callback.ExecuteIfBound(HttpResponsePtr->GetResponseCode() == 200, OutData, FWebResponse(HttpResponsePtr->GetResponseCode(), HttpResponsePtr->GetContentAsString()));
	});

	LogEOSVerbose("-------------------------------------------------\n");
	LogEOSVerbose("Sending SubmitTicket Request (%s)", *HttpRequest->GetURL());
	for (auto& Element : HttpRequest->GetAllHeaders())
	{
		LogEOSVerbose("Header: %s", *Element);
	}
	LogEOSVerbose("Content: %s", *ContentString);
	LogEOSVerbose("-------------------------------------------------\n");

	HttpRequest->ProcessRequest();
}