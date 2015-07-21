#pragma once

#include <ppltasks.h>	// For create_task
#include <fstream>
#include <iterator>
#include <string>

namespace DX
{
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			// Set a breakpoint on this line to catch Win32 API errors.
			throw std::runtime_error(std::to_string(hr));
		}
	}

	inline std::vector<byte> ReadFile(const std::string& filename)
	{
		std::ifstream ifs(filename, std::ios_base::binary);
		if (!ifs)
		{
			throw std::runtime_error("Couldn't open file: " + filename);
		}
		ifs.unsetf(std::ios::skipws);
		std::istream_iterator<byte> begin(ifs), end;
		return std::vector<byte>(begin, end);
	}

	/*// Function that reads from a binary file asynchronously.
	inline Concurrency::task<std::vector<byte>> ReadDataAsync(const std::string& filename)
	{
		return Concurrency::create_task([&filename]() -> std::vector<byte>
		{
			std::ifstream ifs(filename);
			std::istream_iterator<byte> begin(ifs), end;
			return std::vector<byte>(begin, end);
		});
	}*/

#if defined(_DEBUG)
	// Check for SDK Layer support.
	inline bool SdkLayersAvailable()
	{
		HRESULT hr = D3D11CreateDevice(
			nullptr,
			D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
			0,
			D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
			nullptr,                    // Any feature level will do.
			0,
			D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
			nullptr,                    // No need to keep the D3D device reference.
			nullptr,                    // No need to know the feature level.
			nullptr                     // No need to keep the D3D device context reference.
			);

		return SUCCEEDED(hr);
	}
#endif
}
