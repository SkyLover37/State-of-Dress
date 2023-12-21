#include "RenderManager.h"

#include <d3d11.h>

#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <RE/B/BSRenderManager.h>
#include <dxgi.h>

#include "imgui_internal.h"
// stole this from MaxSu's detection meter
#include "lib/imgui_freetype.h"

#include "Rendering/TextureManager.h"

#include "Animation/TimeInterpolator/TimeInterpolatorManager.h"


namespace stl
{
	using namespace SKSE::stl;

	template <class T>
	void write_thunk_call()
	{
		auto& trampoline = SKSE::GetTrampoline();
		const REL::Relocation<std::uintptr_t> hook{ T::id, T::offset };
		T::func = trampoline.write_call<5>(hook.address(), T::thunk);
	}
}


LRESULT RenderManager::WndProcHook::thunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto& io = ImGui::GetIO();
	if (uMsg == WM_KILLFOCUS) {
		io.ClearInputCharacters();
		io.ClearInputKeys();
	}

	return func(hWnd, uMsg, wParam, lParam);
}

void RenderManager::D3DInitHook::thunk()
{
	func();

	logger::info("RenderManager: Initializing...");
	auto render_manager = RE::BSRenderManager::GetSingleton();
	if (!render_manager) {
		logger::error("Cannot find render manager. Initialization failed!");
		return;
	}

	auto render_data = render_manager->GetRuntimeData();

	logger::info("Getting swapchain...");
	auto swapchain = render_data.swapChain;
	if (!swapchain) {
		logger::error("Cannot find swapchain. Initialization failed!");
		return;
	}

	logger::info("Getting swapchain desc...");
	DXGI_SWAP_CHAIN_DESC sd{};
	if (swapchain->GetDesc(std::addressof(sd)) < 0) {
		logger::error("IDXGISwapChain::GetDesc failed.");
		return;
	}

	device = render_data.forwarder;
	Texture::device_ = device;
	context = render_data.context;

	logger::info("Initializing ImGui...");
	ImGui::CreateContext();
	if (!ImGui_ImplWin32_Init(sd.OutputWindow)) {
		logger::error("ImGui initialization failed (Win32)");
		return;
	}
	if (!ImGui_ImplDX11_Init(device, context)) {
		logger::error("ImGui initialization failed (DX11)");
		return;
	}

	logger::info("...ImGui Initialized");

	initialized.store(true);

	WndProcHook::func = reinterpret_cast<WNDPROC>(
		SetWindowLongPtrA(
			sd.OutputWindow,
			GWLP_WNDPROC,
			reinterpret_cast<LONG_PTR>(WndProcHook::thunk)));
	if (!WndProcHook::func)
		logger::error("SetWindowLongPtrA failed!");

	logger::info("Building font atlas...");
	std::filesystem::path fontPath;
	bool foundCustomFont = false;
	const ImWchar* glyphRanges = 0;
#define FONTSETTING_PATH "Data\\SKSE\\Plugins\\wheeler\\resources\\fonts\\FontConfig.ini"
	//CSimpleIniA ini;
	//ini.LoadFile(FONTSETTING_PATH);
	//if (!ini.IsEmpty()) {
		//const char* language = ini.GetValue("config", "font", 0);
		//if (language) {
			std::string fontDir = "";  // R"(Data\SKSE\Plugins\wheeler\resources\fonts\)" + std::string(language);
			// check if folder exists
			if (std::filesystem::exists(fontDir) && std::filesystem::is_directory(fontDir)) {
				for (const auto& entry : std::filesystem::directory_iterator(fontDir)) {
					auto entryPath = entry.path();
					if (entryPath.extension() == ".ttf" || entryPath.extension() == ".ttc") {
						fontPath = entryPath;
						foundCustomFont = true;
						break;
					}
				}
			}
			//if (foundCustomFont) {
				/*std::string languageStr = language;
                logger::info("Loading font: {}", fontPath.string().c_str());
				if (languageStr == "Chinese") {
                    logger::info("Glyph range set to Chinese");
					glyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesChineseFull();
				} else if (languageStr == "Korean") {
                    logger::info("Glyph range set to Korean");
					glyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesKorean();
				} else if (languageStr == "Japanese") {
                    logger::info("Glyph range set to Japanese");
					glyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesJapanese();
				} else if (languageStr == "Thai") {
                    logger::info("Glyph range set to Thai");
					glyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesThai();
				} else if (languageStr == "Vietnamese") {
                    logger::info("Glyph range set to Vietnamese");
					glyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesVietnamese();
				} else if (languageStr == "Cyrillic") {
					glyphRanges = ImGui::GetIO().Fonts->GetGlyphRangesCyrillic();
                    logger::info("Glyph range set to Cyrillic");
				}*/
			//} else {
            //    logger::info("No font found for language: {}", language);
			//}
		//}
	//}
#define ENABLE_FREETYPE 1
#if ENABLE_FREETYPE
	ImFontAtlas* atlas = ImGui::GetIO().Fonts;
	atlas->FontBuilderIO = ImGuiFreeType::GetBuilderForFreeType();
	atlas->FontBuilderFlags = ImGuiFreeTypeBuilderFlags_LightHinting;
#else
#endif
	if (foundCustomFont) {
		ImGui::GetIO().Fonts->AddFontFromFileTTF(fontPath.string().c_str(), 64.0f, NULL, glyphRanges);
	}
	
	logger::info("...font atlas built");

	logger::info("RenderManager: Initialized");

}

void RenderManager::DXGIPresentHook::thunk(std::uint32_t a_p1)
{
	func(a_p1);

	if (!D3DInitHook::initialized.load())
		return;

	// prologue
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// do stuff
	RenderManager::draw();

	// epilogue
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

struct ImageSet
{
	std::int32_t my_image_width = 0;
	std::int32_t my_image_height = 0;
	ID3D11ShaderResourceView* my_texture = nullptr;
};


void RenderManager::MessageCallback(SKSE::MessagingInterface::Message* msg)  //CallBack & LoadTextureFromFile should called after resource loaded.
{
	if (msg->type == SKSE::MessagingInterface::kDataLoaded && D3DInitHook::initialized) {
		auto& io = ImGui::GetIO();
		io.MouseDrawCursor = true;
		io.WantSetMousePos = true;
	}
}

bool RenderManager::Install()
{
	auto g_message = SKSE::GetMessagingInterface();
	if (!g_message) {
		logger::error("Messaging Interface Not Found!");
		return false;
	}

	g_message->RegisterListener(MessageCallback);

	SKSE::AllocTrampoline(14 * 2);

	stl::write_thunk_call<D3DInitHook>();
	stl::write_thunk_call<DXGIPresentHook>();

	
	return true;
}



float RenderManager::GetResolutionScaleWidth()
{
	return ImGui::GetIO().DisplaySize.x / 1920.f;
}

float RenderManager::GetResolutionScaleHeight()
{
	return ImGui::GetIO().DisplaySize.y / 1080.f;
}


void RenderManager::draw()
{

	// Add UI elements here
	float deltaTime = ImGui::GetIO().DeltaTime;
	//Wheeler::Update(deltaTime);
    //logger::debug("call draw");
    /*const float RADIUS_MIN = 30.0f;
    const float RADIUS_MAX = 120.0f;
    const float RADIUS_INTERACT_MIN = 20.0f;
    const int ITEMS_MIN = 6;

    ImGui::SetNextWindowPos(ImVec2(-100, -100));
    if (ImGui::BeginPopup("mywindow")) {
        logger::info("Popup made");

        const ImGuiStyle& style = ImGui::GetStyle();
        auto* font = ImGui::GetDefaultFont();
        auto drawList = ImGui::GetWindowDrawList();

		drawList->PushClipRectFullScreen();
        drawList->PathArcTo(ImGui::GetMousePos(), 10.f, 100.f, 150.f, 7);
        drawList->PathStroke(ImColor(0, 0, 0), true, RADIUS_MAX - RADIUS_MIN);
        
        ImGui::EndPopup();
    } else {
        logger::error("popup failed");
	}
    if (!ImGui::IsPopupOpen("mywindow")) ImGui::OpenPopup("mywindow");*/
    //ImGui::SetNextWindowPos(ImVec2(-100, -100));
	TimeFloatInterpolatorManager::Update(deltaTime);

}
