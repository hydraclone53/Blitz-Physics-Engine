#include "Window.h"
#include <sstream>
#include "Mouse.h"
#include "Resource.h"
#include "Define.h"
#include "imgui/imgui_impl_win32.h"


// Window Class Stuff
Window::WindowClass Window::WindowClass::wndClass;

Window::WindowClass::WindowClass() noexcept
	:
	hInst(GetModuleHandle(nullptr))
{
	WNDCLASSEXW wc = {
	.cbSize = sizeof(wc),
	.style = CS_OWNDC,
	.lpfnWndProc = HandleMsgSetup,
	.hInstance = GetInstance(),
	.lpszClassName = GetName()
	// Everything else is automatically set to 0 by the compiler!
	};
	/*WNDCLASSEXW wc = {0};
	wc.cbSize = sizeof(wc);
	wc.style = CS_OWNDC;
	wc.lpfnWndProc = HandleMsgSetup;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = GetInstance();
	wc.hIcon = nullptr;
	wc.hCursor = nullptr;
	wc.hbrBackground = nullptr;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = GetName();
	wc.hIconSm = nullptr;*/
	RegisterClassExW(&wc);
}
Window::WindowClass::~WindowClass()
{
	UnregisterClassW(GetName(), GetInstance());
}
const wchar_t* Window::WindowClass::GetName() noexcept
{
	return wndClassName;
}
HINSTANCE Window::WindowClass::GetInstance() noexcept
{
	return wndClass.hInst;
}


// Window Stuff
Window::Window(int width, int height, const wchar_t* name)
	:
	width(width),
	height(height)
{
	// calculate window size based on desired client region size
	RECT wr;
	wr.left = 100;
	wr.right = width + wr.left;
	wr.top = 100;
	wr.bottom = height + wr.top;
	if (AdjustWindowRect(&wr, WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU, FALSE) == 0)
	{
		throw CHWND_LAST_EXCEPT();
	}

	hWnd = CreateWindowW(
		WindowClass::GetName(),
		name,
		WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, wr.right - wr.left, wr.bottom - wr.top,
		nullptr, nullptr, WindowClass::GetInstance(), this
	);
	if (hWnd ==nullptr)
	{
		throw CHWND_LAST_EXCEPT();

	}
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	// register mouse raw input device
	RAWINPUTDEVICE rid;
	rid.usUsagePage = 0x01; // mouse page
	rid.usUsage = 0x02; // mouse usage
	rid.dwFlags = 0;
	rid.hwndTarget = nullptr;
	if (RegisterRawInputDevices(&rid, 1, sizeof(rid)) == FALSE)
	{
		throw CHWND_LAST_EXCEPT();
	}

	try
	{
		pGfx = std::make_unique<Graphics>(hWnd);
	}
	catch (...)
	{
		// If graphics fails, destroy the zombie window so it can't receive mouse messages
		DestroyWindow(hWnd);
		throw; // Escalate the exact same exception up to WinMain
	}
}
Window:: ~Window()
	{
		
		DestroyWindow(hWnd);
	}
void Window::SetTitle(const std::wstring& title)
{
	if (SetWindowTextW(hWnd, title.c_str()) == 0)
	{
		throw CHWND_LAST_EXCEPT();
	}
}
void Window::EnableCursor() noexcept
{
	cursorEnabled = true;
	ShowCursor();
	EnableImGuiMouse();
	FreeCursor();
}
void Window::DisableCursor() noexcept
{
	cursorEnabled = false;
	HideCursor();
	DisableImGuiMouse();
	ConfineCursor();
}
bool Window::CursorEnabled() const noexcept
{
	return cursorEnabled;
}

std::optional<int> Window::ProcessMessages()
{
	MSG msg;
	while ((PeekMessage(&msg, nullptr, 0, 0,PM_REMOVE)) > 0)
	{
		if (msg.message == WM_QUIT)
		{
			return static_cast<int>(msg.wParam);
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);

	}	
	return {};
}
Graphics& Window::Gfx()
{
	// TODO: insert return statement here
	return *pGfx;
}
void Window::ConfineCursor() noexcept
{
	RECT rect;
	GetClientRect(hWnd, &rect);
	MapWindowPoints(hWnd, nullptr, reinterpret_cast<POINT*>(&rect), 2);
	ClipCursor(&rect);
}
void Window::FreeCursor() noexcept
{
	ClipCursor(nullptr);
}

void Window::HideCursor() noexcept
{
	while (::ShowCursor(FALSE) >= 0);
}
void Window::ShowCursor() noexcept
{
	while (::ShowCursor(TRUE) < 0);
}
void Window::EnableImGuiMouse() noexcept
{
	ImGui::GetIO().ConfigFlags &= ~ImGuiConfigFlags_NoMouse;
}
void Window::DisableImGuiMouse() noexcept
{
	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NoMouse;
}

LRESULT CALLBACK Window::HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
	{
		// use create parameter passed in from CreateWindow() to store window class pointer at WinAPI side
		if (msg == WM_NCCREATE)
		{
			// extract ptr to window class from creation data
			const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
			Window* const pWnd = static_cast<Window*>(pCreate->lpCreateParams);
			// set WinAPI-managed user data to store ptr to window instance
			SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
			// set message proc to normal (non-setup) handler now that setup is finished
			SetWindowLongPtrW(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Window::HandleMsgThunk));
			// forward message to window instance handler
			return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
		}
		// if we get a message before the WM_NCCREATE message, handle with default handler
		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}
LRESULT CALLBACK Window::HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
	{
		// retrieve ptr to window instance
		Window* const pWnd = reinterpret_cast<Window*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
		// forward message to window instance handler
		return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
	}
LRESULT Window::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) noexcept
	{
	if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
	{
		return true;
		}
	switch (msg)
		{
			// we don't want the DefProc to handle this message because
			// we want our destructor to destroy the window, so return 0 instead of break
		case WM_CLOSE:
			PostQuitMessage(0);
			return 0;

			//Keyboard messages
		case WM_KILLFOCUS:
			kbd.ClearState();
			break;
		case WM_ACTIVATE:
			// confine/free cursor on window to foreground/background if cursor disabled
			if (!cursorEnabled)
			{
				if (wParam & WA_ACTIVE)
				{
					ConfineCursor();
					HideCursor();
				}
				else
				{
					FreeCursor();
					ShowCursor();
				}
			}
			break;

		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (ImGui::GetIO().WantCaptureKeyboard) {
				break;
			}
			if (!(lParam & 0x40000000) || kbd.AutoRepeatIsEnabled())
			{
				kbd.OnKeyPressed(static_cast<unsigned char>(wParam));
			}
			break;

		case WM_KEYUP:
		case WM_SYSKEYUP:
			kbd.OnKeyReleased(static_cast<unsigned char>(wParam));
			break;

		case WM_CHAR:
			kbd.OnChar(static_cast<unsigned char>(wParam));
			break;
			//mouse messages
		case WM_MOUSEMOVE:
		{
			const POINTS pt = MAKEPOINTS(lParam);
			// cursorless exclusive gets first dibs
			if (!cursorEnabled)
			{
				if (!mouse.IsInWindow())
				{
					SetCapture(hWnd);
					mouse.OnMouseEnter();
					HideCursor();
				}
				break;
			}

			
			if (pt.x >= 0 && pt.x < width && pt.y >= 0 && pt.y < height)
			{
				mouse.OnMouseMove(pt.x, pt.y);
				if (!mouse.IsInWindow())
				{
					SetCapture(hWnd);
					mouse.OnMouseEnter();
				}
			}
			else
			{
				if (wParam & (MK_LBUTTON | MK_RBUTTON))
				{
					mouse.OnMouseMove(pt.x, pt.y);
				}
				else
				{
					ReleaseCapture();
					mouse.OnMouseLeave();
				}
				SetCursor(LoadCursor(nullptr, IDC_ARROW));
			}
			break;
		}
			
		case WM_LBUTTONDOWN:
		{	
			if (!cursorEnabled)
			{
				OutputDebugString("lclick => recapture\n");
				ConfineCursor();
				HideCursor();
			}
			const POINTS pt = MAKEPOINTS(lParam);
			mouse.OnLeftPressed(pt.x, pt.y);
			break;
		}

		case WM_RBUTTONDOWN:
		{ 
			const POINTS pt = MAKEPOINTS(lParam);
			mouse.OnRightPressed(pt.x, pt.y);
			break;
		}
		case WM_LBUTTONUP:
		{
			const POINTS pt = MAKEPOINTS(lParam);
			mouse.OnLeftReleased(pt.x, pt.y);
			break;
		}
		case WM_RBUTTONUP:
		{
			const POINTS pt = MAKEPOINTS(lParam);
			mouse.OnRightReleased(pt.x, pt.y);
			break;
		}
		case WM_MOUSEWHEEL:
		{
			// stifle this mouse message if imgui wants to capture
			const POINTS pt = MAKEPOINTS(lParam);
			const int delta = GET_WHEEL_DELTA_WPARAM(wParam);
			mouse.OnWheelDelta(pt.x, pt.y, delta);
			break;
		}
		//raw mouse msgs
		case WM_INPUT:
		{
			if (!mouse.RawEnabled())
			{
				break;
			}

			UINT size;
			// first get the size of the input data
			if (GetRawInputData(
				reinterpret_cast<HRAWINPUT>(lParam),
				RID_INPUT,
				nullptr,
				&size,
				sizeof(RAWINPUTHEADER)) == -1)
			{
				// bail msg processing if error
				break;
			}
			rawBuffer.resize(size);
			// read in the input data
			if (GetRawInputData(
				reinterpret_cast<HRAWINPUT>(lParam),
				RID_INPUT,
				rawBuffer.data(),
				&size,
				sizeof(RAWINPUTHEADER)) != size)
			{
				// bail msg processing if error
				break;
			}
			// process the raw input data
			auto& ri = reinterpret_cast<const RAWINPUT&>(*rawBuffer.data());
			if (ri.header.dwType == RIM_TYPEMOUSE &&
				(ri.data.mouse.lLastX != 0 || ri.data.mouse.lLastY != 0))
			{
				mouse.OnRawDelta(ri.data.mouse.lLastX, ri.data.mouse.lLastY);
			}
			break;
		}
		}
		

		return DefWindowProcW(hWnd, msg, wParam, lParam);
	}
HWND Window::GetHandle() const noexcept
	{
		return hWnd;
	}

	//Exception stuff
	Window::Exception::Exception(int line, const char* file, HRESULT hr) noexcept
		:
		HydraException(line, file),
		hr(hr)
	{
	}
	const char* Window::Exception::what() const noexcept
	{
		std::ostringstream oss;
		oss << GetType() << std::endl
			<< "[Error Code] 0x" << GetErrorCode()<< std::endl
			<< "[Description] " << GetErrorString() << std::endl	
			<< GetOriginString();
		whatBuffer = oss.str();
		return whatBuffer.c_str();
	}

	const char* Window::Exception::GetType() const noexcept
	{
		return "Hydra Window Exception";
	}

	std::string Window::Exception::TranslateErrorCode(HRESULT hr) noexcept
	{
		char* pMsgBuff = nullptr;
		DWORD nMsgLen = FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			nullptr, hr, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			reinterpret_cast<LPSTR>(&pMsgBuff), 0, nullptr

		);
		// 0 string length returned indicates a failure
		if (nMsgLen == 0)
		{
			return "Unidentified error code";
		}
		// copy error string from windows-allocated buffer to std::string
		std::string errorString = pMsgBuff;
		// free windows buffer
		LocalFree(pMsgBuff);
		return errorString;
	}

	HRESULT Window::Exception::GetErrorCode() const noexcept
	{
		return hr;
	}

	std::string Window::Exception::GetErrorString() const noexcept
	{
		return Exception::TranslateErrorCode(hr);
	}