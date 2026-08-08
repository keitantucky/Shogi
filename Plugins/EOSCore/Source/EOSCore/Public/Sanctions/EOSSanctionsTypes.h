/**
* Copyright (C) 2017-2025 | eelDev AB
*
* EOSCore Documentation: https://eeldev.com
*/

#pragma once

#include "CoreMinimal.h"
#include "eos_sanctions_types.h"
#include "Core/EOSHelpers.h"
#include "EOSSanctionsTypes.generated.h"

class UCoreSanctions;

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
//		ENUMS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

UENUM(BlueprintType)
enum class EOSESanctionAppealReason : uint8
{
	/** Not used */
	EOS_SAR_Invalid = 0,
	/** Incorrectly placed sanction */
	EOS_SAR_IncorrectSanction = 1,
	/** The account was compromised, typically this means stolen */
	EOS_SAR_CompromisedAccount = 2,
	/** The punishment is considered too severe by the user */
	EOS_SAR_UnfairPunishment = 3,
	/** The user admits to rulebreaking, but still appeals for forgiveness */
	EOS_SAR_AppealForForgiveness = 4
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
//		STRUCTS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
/**
* Contains information about a single player sanction.
*/
USTRUCT(BlueprintType)
struct FEOSSanctionsPlayerSanction
{
	GENERATED_BODY()
public:
	/** API Version. */
	int32 ApiVersion;
public:
	/** The POSIX timestamp when the sanction was placed */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sanctions")
	FDateTime TimePlaced;
	/** The action associated with this sanction */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sanctions")
	FString Action;
public:
	explicit FEOSSanctionsPlayerSanction()
		: ApiVersion(EOS_SANCTIONS_PLAYERSANCTION_API_LATEST)
		, TimePlaced(0)
	{
	}

	FEOSSanctionsPlayerSanction(const EOS_Sanctions_PlayerSanction& data)
		: ApiVersion(EOS_SANCTIONS_PLAYERSANCTION_API_LATEST)
		, TimePlaced(data.TimePlaced)
	{
		const FUTF8ToTCHAR ActionChar(data.Action);
		Action = ActionChar.Get();
	}
};

/**
* Input parameters for the EOS_Sanctions_QueryActivePlayerSanctions API.
*/
USTRUCT(BlueprintType)
struct FEOSSanctionsQueryActivePlayerSanctionsOptions
{
	GENERATED_BODY()
public:
	/** API Version. */
	int32 ApiVersion;
public:
	/** Product User ID of the user whose active sanctions are to be retrieved. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sanctions")
	FEOSProductUserId TargetUserId;
	/** The Product User ID of the local user who initiated this request. Dedicated servers should set this to null. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sanctions")
	FEOSProductUserId LocalUserId;
public:
	explicit FEOSSanctionsQueryActivePlayerSanctionsOptions()
		: ApiVersion(EOS_SANCTIONS_QUERYACTIVEPLAYERSANCTIONS_API_LATEST)
	{
	}
};

/**
* Output parameters for the EOS_Sanctions_QueryActivePlayerSanctions function.
*/
USTRUCT(BlueprintType)
struct FEOSSanctionsQueryActivePlayerSanctionsCallbackInfo
{
	GENERATED_BODY()
public:
	/** The EOS_EResult code for the operation. EOS_Success indicates that the operation succeeded; other codes indicate errors. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lobby")
	EOSResult ResultCode;
	/** Context that was passed into EOS_Sanctions_QueryActivePlayerSanctions. */
	void* ClientData;
	/** Target Product User ID that was passed to EOS_Sanctions_QueryActivePlayerSanctions. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lobby")
	FEOSProductUserId TargetUserId;
	/** The Product User ID of the local user who initiated this request, if applicable. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lobby")
	FEOSProductUserId LocalUserId;
public:
	FEOSSanctionsQueryActivePlayerSanctionsCallbackInfo()
		: ResultCode(EOSResult::EOS_UnexpectedError)
		, ClientData(nullptr)
	{
	}

	FEOSSanctionsQueryActivePlayerSanctionsCallbackInfo(const EOS_Sanctions_QueryActivePlayerSanctionsCallbackInfo& data)
		: ResultCode(EOSHelpers::ToEOSCoreResult(data.ResultCode))
		, ClientData(data.ClientData)
		, TargetUserId(data.TargetUserId)
		, LocalUserId(data.LocalUserId)
	{
	}
};

/**
* Input parameters for the EOS_Sanctions_GetPlayerSanctionCount function.
*/
USTRUCT(BlueprintType)
struct FEOSSanctionsGetPlayerSanctionCountOptions
{
	GENERATED_BODY()
public:
	/** API Version. */
	int32 ApiVersion;
public:
	/** Product User ID of the user whose sanction count should be returned */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sanctions")
	FEOSProductUserId TargetUserId;
public:
	explicit FEOSSanctionsGetPlayerSanctionCountOptions()
		: ApiVersion(EOS_SANCTIONS_GETPLAYERSANCTIONCOUNT_API_LATEST)
	{
	}
};

/**
* Input parameters for the EOS_Sanctions_CopyPlayerSanctionByIndex function
*/
USTRUCT(BlueprintType)
struct FEOSSanctionsCopyPlayerSanctionByIndexOptions
{
	GENERATED_BODY()
public:
	/** API Version. */
	int32 ApiVersion;
public:
	/** Product User ID of the user whose active sanctions are to be copied */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sanctions")
	FEOSProductUserId TargetUserId;
	/** Index of the sanction to retrieve from the cache */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sanctions")
	int32 SanctionIndex;
public:
	explicit FEOSSanctionsCopyPlayerSanctionByIndexOptions()
		: ApiVersion(EOS_SANCTIONS_COPYPLAYERSANCTIONBYINDEX_API_LATEST)
		, SanctionIndex(0)
	{
	}
};

/**
 * Input parameters for the EOS_Sanctions_CreatePlayerSanctionAppeal function.
 */
USTRUCT(BlueprintType)
struct FEOSSanctionsCreatePlayerSanctionAppealOptions
{
	GENERATED_BODY()
public:
	/** The Product User ID of the local user sending their own sanction appeal. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sanctions")
	FEOSProductUserId LocalUserId;
	/** Reason code for the appeal. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sanctions")
	EOSESanctionAppealReason Reason;
	/** A unique identifier for the specific sanction */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Sanctions")
	FString ReferenceId;
public:
	explicit FEOSSanctionsCreatePlayerSanctionAppealOptions()
		: Reason(EOSESanctionAppealReason::EOS_SAR_Invalid)
	{
	}
};

USTRUCT(BlueprintType)
struct FEOSSanctionsCreatePlayerSanctionAppealCallbackInfo
{
	GENERATED_BODY()
public:
	/** The EOS_EResult code for the operation. EOS_Success indicates that the operation succeeded; other codes indicate errors. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lobby")
	EOSResult ResultCode;
	/** A unique identifier for the specific sanction that was appealed */
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Lobby")
	FString ReferenceId;
public:
	FEOSSanctionsCreatePlayerSanctionAppealCallbackInfo()
		: ResultCode(EOSResult::EOS_UnexpectedError)
	{
	}

	FEOSSanctionsCreatePlayerSanctionAppealCallbackInfo(const EOS_Sanctions_CreatePlayerSanctionAppealCallbackInfo& Data)
		: ResultCode(EOSHelpers::ToEOSCoreResult(Data.ResultCode))
	{
		const FUTF8ToTCHAR ConvertedReferenceId(Data.ReferenceId);
		ReferenceId = ConvertedReferenceId.Get();
	}
};

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
//		DELEGATES
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

DECLARE_DYNAMIC_DELEGATE_OneParam(FOnQueryActivePlayerSanctionsCallback, const FEOSSanctionsQueryActivePlayerSanctionsCallbackInfo&, data);
DECLARE_DYNAMIC_DELEGATE_OneParam(FOnCreatePlayerSanctionAppealCallback, const FEOSSanctionsCreatePlayerSanctionAppealCallbackInfo&, data);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQueryActivePlayerSanctionsCallbackDelegate, const FEOSSanctionsQueryActivePlayerSanctionsCallbackInfo&, data);

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
//		CALLBACK OBJECTS
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

struct FQueryActivePlayerSanctionsCallback
{
public:
	FOnQueryActivePlayerSanctionsCallback m_Callback;
public:
	FQueryActivePlayerSanctionsCallback(const FOnQueryActivePlayerSanctionsCallback& callback)
		: m_Callback(callback)
	{
	}
	virtual ~FQueryActivePlayerSanctionsCallback()
	{
		m_Callback.Unbind();
	}
};

struct FCreatePlayerSanctionAppealCallback
{
public:
	FOnCreatePlayerSanctionAppealCallback m_Callback;
public:
	FCreatePlayerSanctionAppealCallback(const FOnCreatePlayerSanctionAppealCallback& callback)
		: m_Callback(callback)
	{
	}
	virtual ~FCreatePlayerSanctionAppealCallback()
	{
		m_Callback.Unbind();
	}
};
