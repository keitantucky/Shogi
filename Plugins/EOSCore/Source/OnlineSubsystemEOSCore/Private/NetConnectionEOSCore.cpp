/**
* Copyright (C) 2017-2025 | eelDev AB
*
* EOSCore Documentation: https://eeldev.com
*/

#include "NetConnectionEOSCore.h"
#include "NetDriverEOSCore.h"
#include "InternetAddrEOSCore.h"
#include "SocketEOSCore.h"
#include "OnlineSubsystemModuleEOSCore.h"
#include "OnlineSubsystemEOSCorePrivatePCH.h"

UNetConnectionEOSCore::UNetConnectionEOSCore(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	  , bIsPassthrough(false)
	  , bHasP2PSession(false)
{
}

void UNetConnectionEOSCore::InitLocalConnection(UNetDriver* InDriver, FSocket* InSocket, const FURL& InURL, EConnectionState InState, int32 InMaxPacket, int32 InPacketOverhead)
{
	LogEOSVerbose("");

	bIsPassthrough = !static_cast<UNetDriverEOSCore*>(InDriver)->bIsUsingP2PSockets || !InURL.Host.StartsWith(EOSCORE_CONNECTION_URL_PREFIX, ESearchCase::IgnoreCase);
	bHasP2PSession = !bIsPassthrough;

	if (bHasP2PSession)
	{
		DisableAddressResolution();
	}

	Super::InitLocalConnection(InDriver, InSocket, InURL, InState, InMaxPacket, InPacketOverhead);
}

void UNetConnectionEOSCore::InitRemoteConnection(UNetDriver* InDriver, FSocket* InSocket, const FURL& InURL, const FInternetAddr& InRemoteAddr, EConnectionState InState, int32 InMaxPacket, int32 InPacketOverhead)
{
	LogEOSVerbose("");

	bIsPassthrough = static_cast<UNetDriverEOSCore*>(InDriver)->bIsPassthrough;
	bHasP2PSession = !bIsPassthrough;

	if (bHasP2PSession)
	{
		DisableAddressResolution();
	}

	Super::InitRemoteConnection(InDriver, InSocket, InURL, InRemoteAddr, InState, InMaxPacket, InPacketOverhead);
}

void UNetConnectionEOSCore::CleanUp()
{
	LogEOSVerbose("");

	Super::CleanUp();

	if (bHasP2PSession)
	{
		DestroyEOSConnection();
	}
}

void UNetConnectionEOSCore::DestroyEOSConnection()
{
	LogEOSVerbose("");

#if UE_VERSION_OLDER_THAN(5,5,4)
	if (!Socket)
	{
		return;
	}
#else
	if (!GetSocket())
	{
		return;
	}
#endif

	if (!bHasP2PSession)
	{
		return;
	}

	bHasP2PSession = false;

	TSharedPtr<FInternetAddrEOSCore> RemoteAddrEOS = StaticCastSharedPtr<FInternetAddrEOSCore>(RemoteAddr);
	if (RemoteAddrEOS.IsValid())
	{
#if UE_VERSION_OLDER_THAN(5,5,4)
		if (!Socket)
		{
			static_cast<FSocketEOSCore*>(Socket)->Close(*RemoteAddrEOS);
		}
#else
		if (!GetSocket())
		{
			static_cast<FSocketEOSCore*>(GetSocket())->Close(*RemoteAddrEOS);
		}
#endif
	}
}
