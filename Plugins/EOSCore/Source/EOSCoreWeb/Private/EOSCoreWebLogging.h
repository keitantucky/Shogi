#pragma once

#if PLATFORM_ANDROID
#include "Android/AndroidPlatformMisc.h"
#define LogEOSDebug(format, ...) FPlatformMisc::LowLevelOutputDebugStringf(TEXT("LOG: [%s] " format), *FString(__FUNCTION__), ##__VA_ARGS__)
#define LogEOSVerbose(format, ...) FPlatformMisc::LowLevelOutputDebugStringf(TEXT("VERBOSE: [%s] " format), *FString(__FUNCTION__), ##__VA_ARGS__)
#define LogEOSVeryVerbose(format, ...) FPlatformMisc::LowLevelOutputDebugStringf(TEXT("VERY-VERBOSE: [%s] " format), *FString(__FUNCTION__), ##__VA_ARGS__)
#define LogEOSError(format, ...) FPlatformMisc::LowLevelOutputDebugStringf(TEXT("ERROR: [%s] " format), *FString(__FUNCTION__), ##__VA_ARGS__)
#define LogEOSWarning(format, ...) FPlatformMisc::LowLevelOutputDebugStringf(TEXT("WARNING: [%s] " format), *FString(__FUNCTION__), ##__VA_ARGS__)
#else
#define LogEOSDebug(format, ...) UE_LOG(LogEOSCoreWeb, Log, TEXT("[%s] " format), *FString(__FUNCTION__), ##__VA_ARGS__)
#define LogEOSVerbose(format, ...) UE_LOG(LogEOSCoreWeb, Verbose, TEXT("[%s] " format), *FString(__FUNCTION__), ##__VA_ARGS__)
#define LogEOSVeryVerbose(format, ...) UE_LOG(LogEOSCoreWeb, VeryVerbose, TEXT("[%s] " format), *FString(__FUNCTION__), ##__VA_ARGS__)
#define LogEOSError(format, ...) UE_LOG(LogEOSCoreWeb, Error, TEXT("[%s] " format), *FString(__FUNCTION__), ##__VA_ARGS__)
#define LogEOSWarning(format, ...) UE_LOG(LogEOSCoreWeb, Warning, TEXT("[%s] " format), *FString(__FUNCTION__), ##__VA_ARGS__)
#endif 