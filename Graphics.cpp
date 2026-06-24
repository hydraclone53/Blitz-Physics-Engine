#include "Graphics.h"
#include <sstream>
#include "Window.h"
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include "imgui/imgui_impl_dx11.h"
#include "imgui/imgui_impl_win32.h"
#pragma comment (lib,"d3d11.lib")
#pragma comment(lib,"D3DCompiler.lib")


namespace wrl = Microsoft::WRL;
namespace dx = DirectX;
Graphics::Graphics(HWND hWnd)
{
	DXGI_SWAP_CHAIN_DESC sd = {}; // Start by zeroing the whole struct
	sd.BufferDesc.Width = 0;      // 0 means "use window size"
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 0;
	sd.BufferDesc.RefreshRate.Denominator = 0;
	sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;    // Required
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.BufferCount = 2;
	sd.OutputWindow = hWnd;
	sd.Windowed = TRUE;           // REQUIRED: Tell DX you want a window
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Flags = 0;

	UINT swapCreateFlags = 0u;
#ifndef NDEBUG
	swapCreateFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif


	HRESULT hr;

	GFX_THROW_INFO(D3D11CreateDeviceAndSwapChain(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		swapCreateFlags,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		&sd,
		&pSwap,
		&pDevice,
		nullptr,
		&pContext
	));
	wrl::ComPtr<ID3D11Resource> pBackBuffer;
	GFX_THROW_INFO(pSwap->GetBuffer(0, __uuidof(ID3D11Resource), &pBackBuffer));
	GFX_THROW_INFO(pDevice->CreateRenderTargetView(pBackBuffer.Get(), nullptr, &pTarget));
	// create depth stensil state
	D3D11_DEPTH_STENCIL_DESC dsDesc = {};
	dsDesc.DepthEnable = TRUE;
	dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
	wrl::ComPtr<ID3D11DepthStencilState> pDSState;
	GFX_THROW_INFO(pDevice->CreateDepthStencilState(&dsDesc, &pDSState));

	// bind depth state
	pContext->OMSetDepthStencilState(pDSState.Get(), 1u);

	// create depth stensil texture
	wrl::ComPtr<ID3D11Texture2D> pDepthStencil;
	D3D11_TEXTURE2D_DESC descDepth = {};
	descDepth.Width = 800u;
	descDepth.Height = 800u;
	descDepth.MipLevels = 1u;
	descDepth.ArraySize = 1u;
	descDepth.Format = DXGI_FORMAT_D32_FLOAT;
	descDepth.SampleDesc.Count = 1u;
	descDepth.SampleDesc.Quality = 0u;
	descDepth.Usage = D3D11_USAGE_DEFAULT;
	descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	GFX_THROW_INFO(pDevice->CreateTexture2D(&descDepth, nullptr, &pDepthStencil));

	// create view of depth stensil texture
	D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = {};
	descDSV.Format = DXGI_FORMAT_D32_FLOAT;
	descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	descDSV.Texture2D.MipSlice = 0u;
	GFX_THROW_INFO(pDevice->CreateDepthStencilView(
		pDepthStencil.Get(), &descDSV, &pDSV
	));

	// bind depth stensil view to OM
	pContext->OMSetRenderTargets(1u, pTarget.GetAddressOf(), pDSV.Get());

	// --- ADD THIS TO FIX THE ERROR ---
	// configure viewport
	D3D11_VIEWPORT vp;
	vp.Width = 800.0f;
	vp.Height = 800.0f;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0.0f;
	vp.TopLeftY = 0.0f;
	pContext->RSSetViewports(1u, &vp);



}
void Graphics::EndFrame()
	{
	ImGui::Render();
	if (imguiEnabled)
	{
			ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	}
		HRESULT hr;
#ifndef NDEBUG
		infoManager.Set();
#endif
		if (FAILED(hr = pSwap->Present(1u, 0u)))
		{
			if (hr == DXGI_ERROR_DEVICE_REMOVED)
			{
				throw GFX_DEVICE_REMOVED_EXCEPT(pDevice->GetDeviceRemovedReason());
			}
			else
			{
				throw GFX_EXCEPT(hr);
			}
		}
}
void Graphics::BeginFrame(float red, float green, float blue) noexcept
{
	// imgui begin frame
	
	
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();


	const float color[] = { red,green,blue,1.0f };
	pContext->ClearRenderTargetView(pTarget.Get(), color);
	pContext->ClearDepthStencilView(pDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0u);
	pContext->OMSetRenderTargets(1u, pTarget.GetAddressOf(), pDSV.Get());
}
Graphics::~Graphics()
{
	

}
void Graphics::EnableImgui() noexcept
{
	imguiEnabled = true;
}
void Graphics::DisableImgui() noexcept
{
	imguiEnabled = false;
}
bool Graphics::IsImguiEnabled() const noexcept
{
	return imguiEnabled;
}
ID3D11DeviceContext* Graphics::GetContext()const noexcept
{
	return pContext.Get();
}

ID3D11Device* Graphics::GetDevice()const  noexcept
{
	return pDevice.Get();
}



//exception stuff
Graphics::HrException::HrException(int line, const char* file, HRESULT hr, std::vector<std::string> infoMsgs) noexcept
		:
		Exception(line, file),
		hr(hr)
	{
	for (const auto& m : infoMsgs) 
	{
		info += m;
		info.push_back('\n');
	}
	if (!info.empty())
	{
		info.pop_back();
	}

	}
const char* Graphics::HrException::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< "[Error Code] 0x" << std::hex << GetErrorCode() << std::dec << " (" << (unsigned long)GetErrorCode() << ")" << std::endl
		<< "[Description] " << GetErrorString() << std::endl;

	if (!info.empty())
	{
		oss << "\n[Error Info]\n" << info << std::endl << std::endl;
	}

	oss << GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}
std::string Graphics::HrException::GetErrorString() const noexcept
	{
		// Reusing your existing code safely!
		return Window::Exception::TranslateErrorCode(hr);
	}
HRESULT Graphics::HrException::GetErrorCode() const noexcept
{
	return hr;
}
const char* Graphics::HrException::GetType() const noexcept
{
	return "Hydra Graphics Exception";
}
const char* Graphics::DeviceRemovedException::GetType() const noexcept
{
	return "Hydra Graphics Exception [Device Removed] (DXGI_ERROR_DEVICE_REMOVED)";
}
std::string Graphics::HrException::GetErrorInfo() const noexcept
{
	return info;
}
Graphics::InfoException::InfoException(int line, const char* file, std::vector<std::string> infoMsgs) noexcept
	:
	Exception(line, file)
{
	// join all info messages with newlines into single string
	for (const auto& m : infoMsgs)
	{
		info += m;
		info.push_back('\n');
	}
	// remove final newline if exists
	if (!info.empty())
	{
		info.pop_back();
	}
}
const char* Graphics::InfoException::what() const noexcept
{
	std::ostringstream oss;
	oss << GetType() << std::endl
		<< "\n[Error Info]\n" << GetErrorInfo() << std::endl << std::endl;
	oss << GetOriginString();
	whatBuffer = oss.str();
	return whatBuffer.c_str();
}
const char* Graphics::InfoException::GetType() const noexcept
{
	return "Chili Graphics Info Exception";
}
std::string Graphics::InfoException::GetErrorInfo() const noexcept
{
	return info;
}

/*void Graphics::DrawTestTriangle(float angle, float x, float y)
{
	HRESULT hr;

	struct Vertex
	{
		struct
		{
			float x;
			float y;
			float z;
		} pos;
	};

	// --- 1. SPHERICAL GEOMETRY GENERATION ---
	const int latDiv = 20;   // Vertical resolution (tessellation)
	const int longDiv = 20;  // Horizontal resolution
	float radius = 0.5f;
	const int radDiv = 20;
	std::vector<Vertex> vertices;

	// Latitude loop (Phi goes from 0 to PI, North Pole to South Pole)
	for (int i = 0; i <= latDiv; ++i)
	{
		float phi = dx::XM_PI * (float)i / (float)latDiv;

		// Longitude loop (Theta goes from 0 to 2*PI, wrapping around the equator)
		for (int j = 0; j <= longDiv; ++j)
		{ 
				
				
				float theta = dx::XM_2PI * (float)j / (float)longDiv;
				Vertex v;
				// Convert Spherical Coordinates to Cartesian (X, Y, Z)

				v.pos.x = radius *sinf(phi)* cosf(theta);
				v.pos.y = radius * sinf(phi) * sinf(theta);
				v.pos.z = radius* cosf(phi);
				vertices.push_back(v);
			

		}
	}

	// --- 2. CREATE VERTEX BUFFER ---
	wrl::ComPtr<ID3D11Buffer> pVertexBuffer;
	D3D11_BUFFER_DESC bd = {};
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.CPUAccessFlags = 0u;
	bd.MiscFlags = 0u;
	bd.ByteWidth = (UINT)(sizeof(Vertex) * vertices.size()); // Size matches vector contents
	bd.StructureByteStride = sizeof(Vertex);

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = vertices.data(); // Points to the raw array inside the vector
	GFX_THROW_INFO(pDevice->CreateBuffer(&bd, &sd, &pVertexBuffer));

	// Bind vertex buffer to pipeline
	const UINT stride = sizeof(Vertex);
	const UINT offset = 0u;
	pContext->IASetVertexBuffers(0u, 1u, pVertexBuffer.GetAddressOf(), &stride, &offset);

	// --- 3. SEW THE INDEX GRID INTO TRIANGLES ---
	std::vector<unsigned short> indices;
	const int numColumns = longDiv + 1; // Number of vertices per horizontal ring

	for (int i = 0; i < latDiv; ++i)
	{
		for (int j = 0; j < longDiv; ++j)
		{
			// Gather the index addresses of 4 neighboring vertices forming a grid square
			unsigned short topLeft = i * numColumns + j;
			unsigned short topRight = topLeft + 1;
			unsigned short bottomLeft = (i + 1) * numColumns + j;
			unsigned short bottomRight = bottomLeft + 1;

			// Triangle 1
			indices.push_back(topLeft);
			indices.push_back(topRight);
			indices.push_back(bottomLeft);

			// Triangle 2
			indices.push_back(topRight);
			indices.push_back(bottomRight);
			indices.push_back(bottomLeft);
		}
	}

	// --- 4. CREATE INDEX BUFFER ---
	wrl::ComPtr<ID3D11Buffer> pIndexBuffer;
	D3D11_BUFFER_DESC ibd = {};
	ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibd.Usage = D3D11_USAGE_DEFAULT;
	ibd.CPUAccessFlags = 0u;
	ibd.MiscFlags = 0u;
	ibd.ByteWidth = (UINT)(sizeof(unsigned short) * indices.size());
	ibd.StructureByteStride = sizeof(unsigned short);

	D3D11_SUBRESOURCE_DATA isd = {};
	isd.pSysMem = indices.data();
	GFX_THROW_INFO(pDevice->CreateBuffer(&ibd, &isd, &pIndexBuffer));

	// bind index buffer
	pContext->IASetIndexBuffer(pIndexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0u);

	pContext->OMSetRenderTargets(1u, pTarget.GetAddressOf(), pDSV.Get());

	// --- 5. CONSTANT BUFFERS, SHADERS, & VIEWPORT (UNCHANGED EXCLUDING DRAW SIZES) ---
	struct ConstantBuffer
	{
		dx::XMMATRIX transform;
	};

	const ConstantBuffer cb =
	{
		{
			dx::XMMatrixTranspose(
				dx::XMMatrixRotationZ(angle) *
				dx::XMMatrixRotationX(angle) *
				dx::XMMatrixTranslation(x, y, 3.0f) *
				dx::XMMatrixPerspectiveLH(1.0f, 1.0f, 0.5f, 10.0f)
			)
		}
	};
	wrl::ComPtr<ID3D11Buffer> pConstantBuffer;
	D3D11_BUFFER_DESC cbd;
	cbd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd.Usage = D3D11_USAGE_DYNAMIC;
	cbd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbd.MiscFlags = 0u;
	cbd.ByteWidth = sizeof(cb);
	cbd.StructureByteStride = 0u;

	D3D11_SUBRESOURCE_DATA csd = {};
	csd.pSysMem = &cb;
	GFX_THROW_INFO(pDevice->CreateBuffer(&cbd, &csd, &pConstantBuffer));

	// bind constant buffer to vertex shader
	pContext->VSSetConstantBuffers(0u, 1u, pConstantBuffer.GetAddressOf());

	struct ConstantBuffer2
	{
		struct
		{
			float r;
			float g;
			float b;
			float a;
		} face_colors[6];
	};
	const ConstantBuffer2 cb2 =
	{
		{
			{1.0f,1.0f,1.0f},
			{1.0f,0.0f,0.0f},
			{0.0f,1.0f,0.0f},
			{0.0f,0.0f,1.0f},
			{1.0f,1.0f,0.0f},
			{0.0f,1.0f,1.0f},
		}
	};
	wrl::ComPtr<ID3D11Buffer> pConstantBuffer2;
	D3D11_BUFFER_DESC cbd2;
	cbd2.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbd2.Usage = D3D11_USAGE_DEFAULT;
	cbd2.CPUAccessFlags = 0u;
	cbd2.MiscFlags = 0u;
	cbd2.ByteWidth = sizeof(cb2);
	cbd2.StructureByteStride = 0u;

	D3D11_SUBRESOURCE_DATA csd2 = {};
	csd2.pSysMem = &cb2;
	GFX_THROW_INFO(pDevice->CreateBuffer(&cbd2, &csd2, &pConstantBuffer2));

	// bind constant buffer to pixel shader
	pContext->PSSetConstantBuffers(0u, 1u, pConstantBuffer2.GetAddressOf());

	// create pixel shader
	wrl::ComPtr<ID3D11PixelShader> pPixelShader;
	wrl::ComPtr<ID3DBlob> pBlob;
	GFX_THROW_INFO(D3DReadFileToBlob(L"PixelShader.cso", &pBlob));
	GFX_THROW_INFO(pDevice->CreatePixelShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pPixelShader));

	// bind pixel shader
	pContext->PSSetShader(pPixelShader.Get(), nullptr, 0u);

	// create vertex shader
	wrl::ComPtr<ID3D11VertexShader> pVertexShader;
	GFX_THROW_INFO(D3DReadFileToBlob(L"VertexShader.cso", &pBlob));
	GFX_THROW_INFO(pDevice->CreateVertexShader(pBlob->GetBufferPointer(), pBlob->GetBufferSize(), nullptr, &pVertexShader));

	// bind vertex shader
	pContext->VSSetShader(pVertexShader.Get(), nullptr, 0u);

	// input (vertex) layout (3d position)
	wrl::ComPtr<ID3D11InputLayout> pInputLayout;
	const D3D11_INPUT_ELEMENT_DESC ied[] =
	{
		{ "Position",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0 },
	};
	GFX_THROW_INFO(pDevice->CreateInputLayout(
		ied, (UINT)std::size(ied),
		pBlob->GetBufferPointer(),
		pBlob->GetBufferSize(),
		&pInputLayout
	));

	// bind vertex layout
	pContext->IASetInputLayout(pInputLayout.Get());

	// Set primitive topology to triangle list
	pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// configure viewport
	D3D11_VIEWPORT vp;
	vp.Width = 800;
	vp.Height = 800;
	vp.MinDepth = 0;
	vp.MaxDepth = 1;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	pContext->RSSetViewports(1u, &vp);

	// --- 6. DYNAMICALLY EXECUTE INDEX DRAW CALL ---
	// 1. Define the Rasterizer Description
	D3D11_RASTERIZER_DESC rd = {};
	rd.FillMode = D3D11_FILL_SOLID; // Try D3D11_FILL_SOLID if you want it filled
	rd.CullMode = D3D11_CULL_NONE;      // <--- THIS TURNS OFF THE VANISHING BACKSIDE!
	rd.FrontCounterClockwise = FALSE;
	rd.DepthClipEnable = TRUE;

	// 2. Create the State Object via your Device
	wrl::ComPtr<ID3D11RasterizerState> pRasterizerState;
	GFX_THROW_INFO(pDevice->CreateRasterizerState(&rd, &pRasterizerState));

	// 3. Bind it to the Device Context
	pContext->RSSetState(pRasterizerState.Get());
	GFX_THROW_INFO_ONLY(pContext->DrawIndexed((UINT)indices.size(), 0u, 0u));
}*/

void Graphics::SetProjection(DirectX::FXMMATRIX proj) noexcept
{
	projection = proj;
}

DirectX::XMMATRIX Graphics::GetProjection() const noexcept
{
	return projection;
}
void Graphics::DrawIndexed(UINT count) noexcept(!IS_DEBUG)
{
	GFX_THROW_INFO_ONLY(pContext->DrawIndexed(count, 0u, 0u));
}


void Graphics::SetCamera(DirectX::FXMMATRIX cam) noexcept
{
	camera = cam;
}

DirectX::XMMATRIX Graphics::GetCamera() const noexcept
{
	return camera;
}