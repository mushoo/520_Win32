#pragma once

#include "Content\Renderer.h"
#include <memory>
#include "DeviceResources.h"


// Win32 functions.
int WINAPI wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow);
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

namespace _520
{
	// Globals.
	HINSTANCE								g_hInst = nullptr;
	HWND									g_hWnd = nullptr;
	std::shared_ptr<DX::DeviceResources>	g_deviceResources = nullptr;
	std::unique_ptr<_520::Renderer>			g_renderer = nullptr;
	DX::StepTimer							g_timer;

	// Application functions.
	HRESULT InitWindow(HINSTANCE hInstance, int nCmdShow);
	bool Render();
	void Update();
}