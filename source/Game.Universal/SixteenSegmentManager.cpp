#include "pch.h"
#include "SixteenSegmentManager.h"


using namespace std;
using namespace DirectX;
using namespace SimpleMath;
using namespace DX;

namespace DirectXGame
{
	shared_ptr<SixteenSegmentManager> SixteenSegmentManager::sInstance = nullptr;

	SixteenSegmentManager::SixteenSegmentManager(const shared_ptr<DX::DeviceResources>& deviceResources, const shared_ptr<DX::Camera>& camera) :
		DrawableGameComponent(deviceResources, camera), mIndexCount(0), mLoadingComplete(false)
	{
		InitializeFunctionPointers();
	}

	SixteenSegmentManager::~SixteenSegmentManager()
	{
		ReleaseDeviceDependentResources();
	}

	shared_ptr<SixteenSegmentManager> SixteenSegmentManager::Init(const shared_ptr<DX::DeviceResources>& deviceResources, const shared_ptr<DX::Camera>& camera)
	{
		if (sInstance == nullptr)
		{
			sInstance = make_shared<SixteenSegmentManager>(deviceResources, camera);
			sInstance->CreateDeviceDependentResources();
		}
		return sInstance;
	}

	shared_ptr<SixteenSegmentManager> SixteenSegmentManager::GetInstance()
	{
		return sInstance;
	}

	void SixteenSegmentManager::InitializeFunctionPointers()
	{
		displaySegment[0]  = &SixteenSegmentManager::seg_0;
		displaySegment[1]  = &SixteenSegmentManager::seg_1;
		displaySegment[2]  = &SixteenSegmentManager::seg_2;
		displaySegment[3]  = &SixteenSegmentManager::seg_3;
		displaySegment[4]  = &SixteenSegmentManager::seg_4;
		displaySegment[5]  = &SixteenSegmentManager::seg_5;
		displaySegment[6]  = &SixteenSegmentManager::seg_6;
		displaySegment[7]  = &SixteenSegmentManager::seg_7;
		displaySegment[8]  = &SixteenSegmentManager::seg_8;
		displaySegment[9]  = &SixteenSegmentManager::seg_9;
		displaySegment[10] = &SixteenSegmentManager::seg_A;
		displaySegment[11] = &SixteenSegmentManager::seg_B;
		displaySegment[12] = &SixteenSegmentManager::seg_C;
		displaySegment[13] = &SixteenSegmentManager::seg_D;
		displaySegment[14] = &SixteenSegmentManager::seg_E;
		displaySegment[15] = &SixteenSegmentManager::seg_F;
	}

	void SixteenSegmentManager::CreateDeviceDependentResources()
	{
		auto loadVSTask = ReadDataAsync(L"ShapeRendererVS.cso");
		auto loadPSTask = ReadDataAsync(L"ShapeRendererPS.cso");

		// After the vertex shader file is loaded, create the shader and input layout.
		auto createVSTask = loadVSTask.then([this](const vector<byte>& fileData) {
			ThrowIfFailed(
				mDeviceResources->GetD3DDevice()->CreateVertexShader(
					&fileData[0],
					fileData.size(),
					nullptr,
					mVertexShader.ReleaseAndGetAddressOf()
				)
			);

			// Create an input layout
			ThrowIfFailed(
				mDeviceResources->GetD3DDevice()->CreateInputLayout(
					VertexPosition::InputElements,
					VertexPosition::InputElementCount,
					&fileData[0],
					fileData.size(),
					mInputLayout.ReleaseAndGetAddressOf()
				)
			);

			CD3D11_BUFFER_DESC constantBufferDesc(sizeof(XMFLOAT4X4), D3D11_BIND_CONSTANT_BUFFER);
			ThrowIfFailed(
				mDeviceResources->GetD3DDevice()->CreateBuffer(
					&constantBufferDesc,
					nullptr,
					mVSCBufferPerObject.ReleaseAndGetAddressOf()
				)
			);
		});

		// After the pixel shader file is loaded, create the shader and constant buffer.
		auto createPSTask = loadPSTask.then([this](const vector<byte>& fileData) {
			ThrowIfFailed(
				mDeviceResources->GetD3DDevice()->CreatePixelShader(
					&fileData[0],
					fileData.size(),
					nullptr,
					mPixelShader.ReleaseAndGetAddressOf()
				)
			);

			CD3D11_BUFFER_DESC constantBufferDesc(sizeof(XMFLOAT4), D3D11_BIND_CONSTANT_BUFFER);
			ThrowIfFailed(
				mDeviceResources->GetD3DDevice()->CreateBuffer(
					&constantBufferDesc,
					nullptr,
					mPSCBufferPerObject.ReleaseAndGetAddressOf()
				)
			);
		});

		(createPSTask && createVSTask).then([this]() {
			// Create a vertex buffer for rendering a box
			D3D11_BUFFER_DESC vertexBufferDesc = { 0 };
			const uint32_t boxVertexCount = 4;
			vertexBufferDesc.ByteWidth = sizeof(VertexPosition) * boxVertexCount;
			vertexBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
			vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
			vertexBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
			ThrowIfFailed(mDeviceResources->GetD3DDevice()->CreateBuffer(&vertexBufferDesc, nullptr, mVertexBuffer.ReleaseAndGetAddressOf()));

			// Create an index buffer for the box (line strip)
			uint32_t indices[] =
			{
				0, 1, 2, 3, 0
			};

			mIndexCount = ARRAYSIZE(indices);

			D3D11_BUFFER_DESC indexBufferDesc = { 0 };
			indexBufferDesc.ByteWidth = sizeof(uint32_t) * mIndexCount;
			indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
			indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

			D3D11_SUBRESOURCE_DATA indexSubResourceData = { 0 };
			indexSubResourceData.pSysMem = indices;
			ThrowIfFailed(mDeviceResources->GetD3DDevice()->CreateBuffer(&indexBufferDesc, &indexSubResourceData, mIndexBuffer.ReleaseAndGetAddressOf()));
			mLoadingComplete = true;
		});
	}

	void SixteenSegmentManager::ReleaseDeviceDependentResources()
	{
		mLoadingComplete = false;
		mVertexShader.Reset();
		mPixelShader.Reset();
		mInputLayout.Reset();
		mVertexBuffer.Reset();
		mVertexBuffer.Reset();
		mVSCBufferPerObject.Reset();
		mPSCBufferPerObject.Reset();
	}

	void SixteenSegmentManager::Render(const DX::StepTimer& timer)
	{
		UNREFERENCED_PARAMETER(timer);

		// Loading is asynchronous. Only draw geometry after it's loaded.
		if (!mLoadingComplete)
		{
			return;
		}

		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();
		//Use line list topology instead for individual segments
		direct3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
		direct3DDeviceContext->IASetInputLayout(mInputLayout.Get());

		static const UINT stride = sizeof(VertexPosition);
		static const UINT offset = 0;
		direct3DDeviceContext->IASetVertexBuffers(0, 1, mVertexBuffer.GetAddressOf(), &stride, &offset);
		direct3DDeviceContext->IASetIndexBuffer(mIndexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		direct3DDeviceContext->VSSetShader(mVertexShader.Get(), nullptr, 0);
		direct3DDeviceContext->PSSetShader(mPixelShader.Get(), nullptr, 0);

		const XMMATRIX wvp = XMMatrixTranspose(mCamera->ViewProjectionMatrix());
		direct3DDeviceContext->UpdateSubresource(mVSCBufferPerObject.Get(), 0, nullptr, reinterpret_cast<const float*>(wvp.r), 0, 0);
		direct3DDeviceContext->VSSetConstantBuffers(0, 1, mVSCBufferPerObject.GetAddressOf());
		direct3DDeviceContext->PSSetConstantBuffers(0, 1, mPSCBufferPerObject.GetAddressOf());
	}

	void SixteenSegmentManager::Render(const DX::StepTimer& timer, uint16_t value, float x, float y)
	{
		Render(timer);
		for (uint32_t i = 0; i < 16; i++)
		{
			bool segmentBit = static_cast<bool>(value & 0x0001);
			if (segmentBit)
			{
				(this->*(this->displaySegment[i]))(segmentBit, x, y);
			}
			value >>= 1;
		}
	}

	void SixteenSegmentManager::Render(const DX::StepTimer& timer, char c, float x, float y)
	{
		Render(timer);
		switch (tolower(c))
		{
		case 'a':
			Render(timer, A, x, y);
			break;
		case 'b':
			Render(timer, B, x, y);
			break;
		case 'c':
			Render(timer, C, x, y);
			break;
		case 'd':
			Render(timer, D, x, y);
			break;
		case 'e':
			Render(timer, E, x, y);
			break;
		case 'f':
			Render(timer, F, x, y);
			break;
		case 'g':
			Render(timer, G, x, y);
			break;
		case 'h':
			Render(timer, H, x, y);
			break;
		case 'i':
			Render(timer, I, x, y);
			break;
		case 'j':
			Render(timer, J, x, y);
			break;
		case 'k':
			Render(timer, K, x, y);
			break;
		case 'l':
			Render(timer, L, x, y);
			break;
		case 'm':
			Render(timer, M, x, y);
			break;
		case 'n':
			Render(timer, N, x, y);
			break;
		case 'o':
			Render(timer, O, x, y);
			break;
		case 'p':
			Render(timer, P, x, y);
			break;
		case 'q':
			Render(timer, Q, x, y);
			break;
		case 'r':
			Render(timer, R, x, y);
			break;
		case 's':
			Render(timer, S, x, y);
			break;
		case 't':
			Render(timer, T, x, y);
			break;
		case 'u':
			Render(timer, U, x, y);
			break;
		case 'v':
			Render(timer, V, x, y);
			break;
		case 'w':
			Render(timer, W, x, y);
			break;
		case 'x':
			Render(timer, X, x, y);
			break;
		case 'y':
			Render(timer, Y, x, y);
			break;
		case 'z':
			Render(timer, Z, x, y);
			break;
		case '0':
			Render(timer, ZERO, x, y);
			break;
		case '1':
			Render(timer, ONE, x, y);
			break;
		case '2':
			Render(timer, TWO, x, y);
			break;
		case '3':
			Render(timer, THREE, x, y);
			break;
		case '4':
			Render(timer, FOUR, x, y);
			break;
		case '5':
			Render(timer, FIVE, x, y);
			break;
		case '6':
			Render(timer, SIX, x, y);
			break;
		case '7':
			Render(timer, SEVEN, x, y);
			break;
		case '8':
			Render(timer, EIGHT, x, y);
			break;
		case '9':
			Render(timer, NINE, x, y);
			break;
		default:
			break;
		}
	}

	void SixteenSegmentManager::DisplayString(const DX::StepTimer& timer, char * word, float x, float y)
	{
		for (int i = 0; i < strlen(word); i++)
		{
			Render(timer, word[i], x + (50 * i), y);
		}
	}

	void SixteenSegmentManager::seg_0(bool on, float x, float y)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			VertexPosition(XMFLOAT4(x + 4, y + 81, 0.0f, 1.0f)),
			VertexPosition(XMFLOAT4(x + 22, y + 81, 0.0f, 1.0f)),
		};

		auto col = on ? SEG_ON : SEG_OFF;
		XMFLOAT4 color(col, col, col, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void SixteenSegmentManager::seg_1(bool on, float x, float y)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			VertexPosition(XMFLOAT4(x + 24, y + 81, 0.0f, 1.0f)),
			VertexPosition(XMFLOAT4(x + 42, y + 81, 0.0f, 1.0f)),
		};

		auto col = on ? SEG_ON : SEG_OFF;
		XMFLOAT4 color(col, col, col, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void SixteenSegmentManager::seg_2(bool on, float x, float y)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			VertexPosition(XMFLOAT4(x + 43, y + 80, 0.0f, 1.0f)),
			VertexPosition(XMFLOAT4(x + 43, y + 43, 0.0f, 1.0f)),
		};

		auto col = on ? SEG_ON : SEG_OFF;
		XMFLOAT4 color(col, col, col, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void SixteenSegmentManager::seg_3(bool on, float x, float y)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			VertexPosition(XMFLOAT4(x + 43, y + 41, 0.0f, 1.0f)),
			VertexPosition(XMFLOAT4(x + 43, y + 4, 0.0f, 1.0f)),
		};

		auto col = on ? SEG_ON : SEG_OFF;
		XMFLOAT4 color(col, col, col, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void SixteenSegmentManager::seg_4(bool on, float x, float y)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			VertexPosition(XMFLOAT4(x + 24, y + 3, 0.0f, 1.0f)),
			VertexPosition(XMFLOAT4(x + 42, y + 3, 0.0f, 1.0f)),
		};

		auto col = on ? SEG_ON : SEG_OFF;
		XMFLOAT4 color(col, col, col, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void SixteenSegmentManager::seg_5(bool on, float x, float y)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			VertexPosition(XMFLOAT4(x + 4, y + 3, 0.0f, 1.0f)),
			VertexPosition(XMFLOAT4(x + 22, y + 3, 0.0f, 1.0f)),
		};

		auto col = on ? SEG_ON : SEG_OFF;
		XMFLOAT4 color(col, col, col, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void SixteenSegmentManager::seg_6(bool on, float x, float y)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			VertexPosition(XMFLOAT4(x + 3, y + 5, 0.0f, 1.0f)),
			VertexPosition(XMFLOAT4(x + 3, y + 41, 0.0f, 1.0f)),
		};

		auto col = on ? SEG_ON : SEG_OFF;
		XMFLOAT4 color(col, col, col, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void SixteenSegmentManager::seg_7(bool on, float x, float y)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			VertexPosition(XMFLOAT4(x + 3, y + 44, 0.0f, 1.0f)),
			VertexPosition(XMFLOAT4(x + 3, y + 80, 0.0f, 1.0f)),
		};

		auto col = on ? SEG_ON : SEG_OFF;
		XMFLOAT4 color(col, col, col, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void SixteenSegmentManager::seg_8(bool on, float x, float y)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			VertexPosition(XMFLOAT4(x + 6, y + 78, 0.0f, 1.0f)),
			VertexPosition(XMFLOAT4(x + 19, y + 44, 0.0f, 1.0f)),
		};

		auto col = on ? SEG_ON : SEG_OFF;
		XMFLOAT4 color(col, col, col, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void SixteenSegmentManager::seg_9(bool on, float x, float y)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			VertexPosition(XMFLOAT4(x + 23, y + 43, 0.0f, 1.0f)),
			VertexPosition(XMFLOAT4(x + 23, y + 80, 0.0f, 1.0f)),
		};

		auto col = on ? SEG_ON : SEG_OFF;
		XMFLOAT4 color(col, col, col, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void SixteenSegmentManager::seg_A(bool on, float x, float y)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			VertexPosition(XMFLOAT4(x + 27, y + 45, 0.0f, 1.0f)),
			VertexPosition(XMFLOAT4(x + 40, y + 78, 0.0f, 1.0f)),
		};

		auto col = on ? SEG_ON : SEG_OFF;
		XMFLOAT4 color(col, col, col, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void SixteenSegmentManager::seg_B(bool on, float x, float y)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			VertexPosition(XMFLOAT4(x + 24, y + 42, 0.0f, 1.0f)),
			VertexPosition(XMFLOAT4(x + 42, y + 42, 0.0f, 1.0f)),
		};

		auto col = on ? SEG_ON : SEG_OFF;
		XMFLOAT4 color(col, col, col, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void SixteenSegmentManager::seg_C(bool on, float x, float y)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			VertexPosition(XMFLOAT4(x + 26, y + 39, 0.0f, 1.0f)),
			VertexPosition(XMFLOAT4(x + 41, y + 5, 0.0f, 1.0f)),
		};

		auto col = on ? SEG_ON : SEG_OFF;
		XMFLOAT4 color(col, col, col, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void SixteenSegmentManager::seg_D(bool on, float x, float y)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			VertexPosition(XMFLOAT4(x + 23, y + 5, 0.0f, 1.0f)),
			VertexPosition(XMFLOAT4(x + 23, y + 40, 0.0f, 1.0f)),
		};

		auto col = on ? SEG_ON : SEG_OFF;
		XMFLOAT4 color(col, col, col, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void SixteenSegmentManager::seg_E(bool on, float x, float y)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			VertexPosition(XMFLOAT4(x + 20, y + 39, 0.0f, 1.0f)),
			VertexPosition(XMFLOAT4(x + 6, y + 5, 0.0f, 1.0f)),
		};

		auto col = on ? SEG_ON : SEG_OFF;
		XMFLOAT4 color(col, col, col, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void SixteenSegmentManager::seg_F(bool on, float x, float y)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			VertexPosition(XMFLOAT4(x + 4, y + 42, 0.0f, 1.0f)),
			VertexPosition(XMFLOAT4(x + 22, y + 42, 0.0f, 1.0f)),
		};

		auto col = on ? SEG_ON : SEG_OFF;
		XMFLOAT4 color(col, col, col, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

}
