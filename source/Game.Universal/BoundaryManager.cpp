#include "pch.h"
#include "BoundaryManager.h"


using namespace std;
using namespace DirectX;
using namespace SimpleMath;
using namespace DX;

namespace DirectXGame
{
	BoundaryManager::BoundaryManager(const std::shared_ptr<DX::DeviceResources>& deviceResources, const std::shared_ptr<DX::Camera>& camera):
		DrawableGameComponent(deviceResources, camera), mIndexCount(0), mLoadingComplete(false)
	{
	}

	void BoundaryManager::CreateDeviceDependentResources()
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

	void BoundaryManager::ReleaseDeviceDependentResources()
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

	void BoundaryManager::Render(const DX::StepTimer & timer)
	{
		UNREFERENCED_PARAMETER(timer);
		// Loading is asynchronous. Only draw geometry after it's loaded.
		if (!mLoadingComplete)
		{
			return;
		}

		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();
		direct3DDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
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

		RenderTop();
		RenderBottom();
		RenderLeft();
		RenderRight();
	}

	void BoundaryManager::RenderTop()
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			// Upper-Left
			VertexPosition(XMFLOAT4(-800.0f, 450.0f, 0.0f, 1.0f)),

			// Upper-Right
			VertexPosition(XMFLOAT4(800.0f, 450.0f, 0.0f, 1.0f)),

			// Lower-Left
			VertexPosition(XMFLOAT4(-800.0f, 450.0f - (5.0f * BodySize), 0.0f, 1.0f)),

			// Lower-Right
			VertexPosition(XMFLOAT4(800.0f, 450.0f - (5.0f * BodySize), 0.0f, 1.0f)),
		};

		XMFLOAT4 color(0.3f, 0.3f, 0.3f, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void BoundaryManager::RenderBottom()
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			// Upper-Left
			VertexPosition(XMFLOAT4(-800.0f, -450.0f + (5.0f * BodySize), 0.0f, 1.0f)),

			// Upper-Right
			VertexPosition(XMFLOAT4(800.0f, -450.0f + (5.0f * BodySize), 0.0f, 1.0f)),

			// Lower-Left
			VertexPosition(XMFLOAT4(-800.0f, -450.0f + (4.0f * BodySize), 0.0f, 1.0f)),

			// Lower-Right
			VertexPosition(XMFLOAT4(800.0f, -450.0f + (4.0f * BodySize), 0.0f, 1.0f)),
		};

		XMFLOAT4 color(0.3f, 0.3f, 0.3f, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void BoundaryManager::RenderLeft()
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			// Upper-Left
			VertexPosition(XMFLOAT4(-800.0f, 450.0f - (5.0f * BodySize), 0.0f, 1.0f)),

			// Upper-Right
			VertexPosition(XMFLOAT4(-800.0f + (5.0f * BodySize), 450.0f - (5.0f * BodySize), 0.0f, 1.0f)),

			// Lower-Left
			VertexPosition(XMFLOAT4(-800.0f, -450.0f + (5.0f * BodySize), 0.0f, 1.0f)),

			// Lower-Right
			VertexPosition(XMFLOAT4(-800.0f + (5.0f * BodySize), -450.0f + (5.0f * BodySize), 0.0f, 1.0f)),
		};

		XMFLOAT4 color(0.3f, 0.3f, 0.3f, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void BoundaryManager::RenderRight()
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			// Upper-Left
			VertexPosition(XMFLOAT4(800.0f - (5.0f * BodySize), 450.0f - (5.0f * BodySize), 0.0f, 1.0f)),

			// Upper-Right
			VertexPosition(XMFLOAT4(800.0f, 450.0f - (5.0f * BodySize), 0.0f, 1.0f)),

			// Lower-Left
			VertexPosition(XMFLOAT4(800.0f - (5.0f * BodySize), -450.0f + (5.0f * BodySize), 0.0f, 1.0f)),

			// Lower-Right
			VertexPosition(XMFLOAT4(800.0f, -450.0f + (5.0f * BodySize), 0.0f, 1.0f)),
		};

		XMFLOAT4 color(0.3f, 0.3f, 0.3f, 1.0f);

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}
}
