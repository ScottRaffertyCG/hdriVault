// Copyright Pyre Labs 2025. All Rights Reserved.

#include "HdriVaultImageUtils.h"
#include "Misc/Paths.h"

// Standard library includes for tinyexr
#include <vector>
#include <string>
#include <algorithm>
#include <cassert>

// Disable warnings for third party libraries
#if defined(_MSC_VER)
	#pragma warning(push)
	#pragma warning(disable: 4996) // CRT secure warnings
	#pragma warning(disable: 4100) // Unreferenced formal parameter
	#pragma warning(disable: 4505) // Unreferenced local function has been removed
	#pragma warning(disable: 4189) // Local variable is initialized but not referenced
    #pragma warning(disable: 4456) // Declaration hides previous local declaration
	#pragma warning(disable: 4244) // Conversion loss of data
#endif

#ifndef TEXR_ASSERT
#define TEXR_ASSERT assert
#endif

#define TINYEXR_IMPLEMENTATION
#include "ThirdParty/tinyexr.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "ThirdParty/stb_image_write.h"

#if defined(_MSC_VER)
	#pragma warning(pop)
#endif

bool FHdriVaultImageUtils::ConvertExrToHdr(const FString& InputFile, const FString& OutputFile, FString& OutError)
{
	float* Rgba = nullptr; // width * height * 4
	int Width = 0;
	int Height = 0;
	const char* Err = nullptr;

	// Load EXR
	// InputFile is FString, TCHAR*. TinyEXR expects char*.
	// On Windows, paths with non-ASCII chars might be tricky with ANSI_TO_TCHAR/TCHAR_TO_ANSI if not handled well, 
	// but for now we assume standard paths or that TCHAR_TO_ANSI works for the file system api used by tinyexr (fopen/ifstream).
	// TinyEXR uses standard C++ ifstream or FILE*, which takes const char* on Windows usually implying ANSI/UTF8 depending on locale.
	// Ideally we'd use the wchar_t version if available or UTF8, but let's try standard conversion.
	
	int Ret = LoadEXR(&Rgba, &Width, &Height, TCHAR_TO_ANSI(*InputFile), &Err);

	if (Ret != TINYEXR_SUCCESS)
	{
		if (Err)
		{
			OutError = FString::Printf(TEXT("TinyEXR Error: %s"), ANSI_TO_TCHAR(Err));
			FreeEXRErrorMessage(Err);
		}
		else
		{
			OutError = TEXT("Unknown TinyEXR Error");
		}
		return false;
	}

	if (!Rgba)
	{
		OutError = TEXT("Failed to load EXR data (null buffer)");
		return false;
	}

	// Save as HDR using stbi_write_hdr
	// stbi_write_hdr expects float* pointing to RGB or RGBA data.
	// Since LoadEXR returns RGBA, we pass 4 components.
	// stbi_write_hdr supports 4 components (it will just write RGBA, though .hdr is usually RGBE, stbi might handle alpha or discard it, 
    // but standard Radiance HDR is RGBE. stbi_write_hdr doc says it supports 1, 2, 3, 4 components).
    
	int WriteRet = stbi_write_hdr(TCHAR_TO_ANSI(*OutputFile), Width, Height, 4, Rgba);

	// Clean up EXR memory
	free(Rgba);

	if (WriteRet == 0)
	{
		OutError = TEXT("Failed to write HDR file using stbi_write_hdr");
		return false;
	}

	return true;
}

