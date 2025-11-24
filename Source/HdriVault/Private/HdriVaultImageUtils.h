// Copyright Pyre Labs. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FHdriVaultImageUtils
{
public:
	/**
	 * Converts an EXR file to HDR format using tinyexr and stb_image_write.
	 * @param InputFile - Full path to the source .exr file
	 * @param OutputFile - Full path to the destination .hdr file
	 * @param OutError - Error message if conversion fails
	 * @return true if successful
	 */
	static bool ConvertExrToHdr(const FString& InputFile, const FString& OutputFile, FString& OutError);
};

