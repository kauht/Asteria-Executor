#include "directx.h"
#include "draw.h"
#include "sdk.h"
bool init = false;
#include "includes.h"
#include "fonts.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
HMODULE g_hModule = nullptr;
Present oPresent;
HWND window = NULL;
WNDPROC oWndProc;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView;

void CleanupRenderTarget()
{
	if (mainRenderTargetView) { mainRenderTargetView->Release(); mainRenderTargetView = nullptr; }
}


void CreateRenderTarget(IDXGISwapChain* pSwapChain)
{
	ID3D11Texture2D* pBackBuffer = nullptr;
	if (SUCCEEDED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer)))
	{
		D3D11_RENDER_TARGET_VIEW_DESC desc{};
		desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DMS;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		pDevice->CreateRenderTargetView(pBackBuffer, &desc, &mainRenderTargetView);
		pBackBuffer->Release();
		pBackBuffer = nullptr;
	}

}
void InitImGui()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags = ImGuiConfigFlags_NoMouseCursorChange;
	
	// Настройки сглаживания для устранения "лесенок"
	ImGuiStyle& style = ImGui::GetStyle();
	style.AntiAliasedLines = true;           // Сглаживание линий
	style.AntiAliasedFill = true;            // Сглаживание заливки
	style.AntiAliasedLinesUseTex = true;     // Использование текстуры для сглаживания линий
	
	// Настройки сглаживания для DirectX
	if (pDevice) {
		// Создаем сэмплер с линейной фильтрацией для сглаживания текстур
		D3D11_SAMPLER_DESC samplerDesc = {};
		samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;  // Линейная фильтрация
		samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
		samplerDesc.MipLODBias = 0.0f;
		samplerDesc.MaxAnisotropy = 1;
		samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		samplerDesc.MinLOD = 0.0f;
		samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
		
		ID3D11SamplerState* samplerState = nullptr;
		pDevice->CreateSamplerState(&samplerDesc, &samplerState);
		if (samplerState) {
			pContext->PSSetSamplers(0, 1, &samplerState);
			samplerState->Release();
		}
	}
	
	//FONTS::Initialize(io);
	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);
	FONTS::Initialize(io);

}
#include "hooks.h"
LRESULT __stdcall WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam);

	if (uMsg == WM_KEYUP && wParam == VK_INSERT) {
		SDK::showMenu = !SDK::showMenu;
		SDK::menuAlphaTarget = SDK::showMenu ? 1.0f : 0.0f;
		if (SDK::showMenu) {
			H::hkEnableCursor.GetOriginal()(I::inputsystem, 0);
			SetCursorPos(SDK::g_ScreenWidth / 2, SDK::g_ScreenHeight / 2);
			
		}
		else {
			H::hkEnableCursor.GetOriginal()(I::inputsystem, SDK::cursorlaststate);


		}


		//SetCursorPos(SDK::g_ScreenWidth / 2, SDK::g_ScreenHeight / 2);
	}

	if (SDK::showMenu) {
		switch (uMsg) {
		case WM_KEYDOWN:
		case WM_CHAR:
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
		case WM_MOUSEWHEEL:
			return 0; 
		}
	}


	return CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam);
}


HRESULT __stdcall hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (!init)
	{
		if (SUCCEEDED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)))
		{
			pDevice->GetImmediateContext(&pContext);
			DXGI_SWAP_CHAIN_DESC sd;
			pSwapChain->GetDesc(&sd);
			window = sd.OutputWindow;
			//ID3D11Texture2D* pBackBuffer;
			CreateRenderTarget(pSwapChain);
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);

			InitImGui();
			init = true;
		}

		else
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	// Update menu alpha animation
	float deltaTime = ImGui::GetIO().DeltaTime;
	SDK::menuAlpha += (SDK::menuAlphaTarget - SDK::menuAlpha) * SDK::menuAnimationSpeed * deltaTime;
	
	// Clamp alpha to valid range
	SDK::menuAlpha = std::clamp(SDK::menuAlpha, 0.0f, 1.0f);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	////RENDER HERE
	DRAW::render();

	//
	ImGui::Render();

	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	//if (resize) {
	//	//g_ScreenHeight = ImGui::GetIO().DisplaySize.y;
	//	// 
	//	// 
	//	//g_ScreenWidth = ImGui::GetIO().DisplaySize.x;
	//	printf("debug: g_ScreenWidth: %d\n", g_ScreenWidth);
	//	printf("debug: g_ScreenHeight: %d\n", g_ScreenHeight);
	//	resize = false;
	//}
	if (SDK::shouldUnload) {
		printf("[-] Unloading DirectX resources...\n");

		SDK::showMenu = false;
		// Cleanup ImGui first
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		
		// Restore original WndProc
		if (window && oWndProc) {
			SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)oWndProc);
		}
		
		// Cleanup DirectX resources
		CleanupRenderTarget();
		if (mainRenderTargetView) {
			mainRenderTargetView->Release();
			mainRenderTargetView = nullptr;
		}
		if (pContext) {
			pContext->Release();
			pContext = nullptr;
		}
		if (pDevice) {
			pDevice->Release();
			pDevice = nullptr;
		}
		
		// Call original present and then unhook
		HRESULT result = oPresent(pSwapChain, SyncInterval, Flags);
		
		// Unhook kiero
		kiero::unbind(8);  // Present
		kiero::unbind(13); // ResizeBuffers
		
		return result;
	}

	return oPresent(pSwapChain, SyncInterval, Flags);
}


ResizeBuffersFn oResizeBuffers = nullptr;
HRESULT __stdcall hkResizeBuffers(IDXGISwapChain* pSwapChain, UINT BufferCount, UINT Width, UINT Height,
    DXGI_FORMAT NewFormat, UINT SwapChainFlags)
{
    ImGui_ImplDX11_InvalidateDeviceObjects();
    CleanupRenderTarget();  

    HRESULT hr = oResizeBuffers(pSwapChain, BufferCount, Width, Height, NewFormat, SwapChainFlags);

    CreateRenderTarget(pSwapChain);  


    ImGui_ImplDX11_CreateDeviceObjects();
    return hr;
}