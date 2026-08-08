/**
* Copyright (C) 2017-2025 | eelDev AB
*
* EOSCore Documentation: https://eeldev.com
*/

#pragma once

#include "CoreMinimal.h"
#include "EOSCoreModule.h"
#include "EOSRTCData.generated.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //
//		UCoreRTCData
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ //

/**
 * The RTC Data Interface. This is used to manage Data specific RTC features
 *
 * @see EOS_RTC_GetDataInterface
 */

UCLASS()
class EOSCORE_API UCoreRTCData : public UEOSCoreSubsystem
{
	GENERATED_BODY()
public:
	virtual void Deinitialize() override;
public:
	/**
	* The RTC Audio Interface. This is used to manage Audio specific RTC features
	*
	* @see EOS_RTC_GetVoiceInterface
	*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "EOSCore|Interfaces", meta = (WorldContext = "WorldContextObject"))
	static UCoreRTCData* GetRTCData(UObject* WorldContextObject);

public:
};
