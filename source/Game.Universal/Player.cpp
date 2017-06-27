#include "pch.h"
#include "Player.h"
#include "SixteenSegmentManager.h"
#include "PowerupManager.h"


using namespace std;
using namespace DirectX;
using namespace SimpleMath;
using namespace DX;

namespace DirectXGame
{
	Player::Player(const shared_ptr<DX::DeviceResources>& deviceResources, const shared_ptr<Camera>& camera) :
		DrawableGameComponent(deviceResources, camera),
		mPosition(0, 0), mTail(0), mVelocity(0, 0),
		mIndexCount(0), mLoadingComplete(false), mTimeSinceUpdate(0.0f),
		mDirection(Direction::Stop), mAlive(true), mGameSpeed(0.25f), mDirectionLocked(false)
	{
		mTail.clear();
	}

	void Player::CreateDeviceDependentResources()
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

	void Player::ReleaseDeviceDependentResources()
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

	void Player::Render(const DX::StepTimer & timer)
	{
		mTimeSinceUpdate += timer.GetElapsedSeconds();

		if (mTimeSinceUpdate >= mGameSpeed && mAlive)
		{
			mPosition += mVelocity;
			mTimeSinceUpdate = 0.0f;
			UpdateTail();
			mDirectionLocked = false;
			HandleCollisions();
		}	
		else if (!mAlive)
		{
			SixteenSegmentManager::GetInstance()->DisplayString(timer, "Game Over", -225, 50);
			SixteenSegmentManager::GetInstance()->DisplayString(timer, "Press Start to Continue", -550, -100);
		}

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

		RenderSquare(mPosition);
		for (auto position : mTail)
		{
			RenderSquare(position);
		}
	}

	void Player::Move(Direction direction)
	{
		switch (direction)
		{
		case Up:
			if (!mDirectionLocked && mDirection == Direction::Left || mDirection == Direction::Right || mDirection == Direction::Stop &&
				(mTail.size() == 0 || !(mTail[0].x == mPosition.x && mTail[0].y == mPosition.y + BodySize)))
			{
				mVelocity = Vector2f(0.0f, static_cast<float>(BodySize));
				mDirection = Direction::Up;
			}
			break;
		case Down:
			if (!mDirectionLocked && mDirection == Direction::Left || mDirection == Direction::Right || mDirection == Direction::Stop &&
				(mTail.size() == 0 || !(mTail[0].x == mPosition.x && mTail[0].y == mPosition.y - BodySize)))
			{
				mVelocity = Vector2f(0.0f, -static_cast<float>(BodySize));
				mDirection = Direction::Down;
			}
			break;
		case Left:
			if (!mDirectionLocked && mDirection == Direction::Up || mDirection == Direction::Down || mDirection == Direction::Stop &&
				(mTail.size() == 0 || !(mTail[0].x == mPosition.x - BodySize && mTail[0].y == mPosition.y)))
			{
				mVelocity = Vector2f(-static_cast<float>(BodySize), 0);
				mDirection = Direction::Left;
			}
			break;
		case Right:
			if (!mDirectionLocked && mDirection == Direction::Up || mDirection == Direction::Down || mDirection == Direction::Stop &&
				(mTail.size() == 0 || !(mTail[0].x == mPosition.x + BodySize && mTail[0].y == mPosition.y)))
			{
				mVelocity = Vector2f(static_cast<float>(BodySize), 0);
				mDirection = Direction::Right;
			}
			break;
		default:
			break;
		}
		mDirectionLocked = true;
	}

	void Player::UpdateTail()
	{ 
		if (mTail.size() <= 0) return;

		Vector2f lastCoord = mTail[0];
		Vector2f last2Coord;
		mTail[0] = mPosition;

		for (uint32_t i = 1; i < mTail.size(); i++)
		{
			last2Coord = mTail[i];
			mTail[i] = lastCoord;
			lastCoord = last2Coord;
		}
	}

	void Player::IncreaseTail()
	{
		for (uint32_t i = 0; i < 10; i++)
		{
			mTail.push_back(Vector2f(-5000, -5000));
		}
		mGameSpeed -= 0.005f;
	}

	uint32_t Player::GetTailSize()
	{
		return static_cast<uint32_t>(mTail.size());
	}

	Vector2f Player::GetTailAt(uint32_t index)
	{
		return mTail[index];
	}

	bool Player::Alive()
	{
		return mAlive;
	}

	void Player::Kill()
	{
		mAlive = !mAlive;
	}
	void Player::Respawn()
	{
		mPosition = Vector2f(0, 0);
		mDirection = Direction::Stop;
	}

	void Player::RenderSquare(Vector2f position)
	{
		ID3D11DeviceContext* direct3DDeviceContext = mDeviceResources->GetD3DDeviceContext();

		VertexPosition vertices[] =
		{
			// Upper-Left
			VertexPosition(XMFLOAT4(position.x + 1, position.y + BodySize - 1, 0.0f, 1.0f)),

			// Upper-Right
			VertexPosition(XMFLOAT4(position.x + BodySize - 1, position.y + BodySize - 1, 0.0f, 1.0f)),

			// Lower-Left
			VertexPosition(XMFLOAT4(position.x + 1, position.y + 1, 0.0f, 1.0f)),

			// Lower-Right
			VertexPosition(XMFLOAT4(position.x + BodySize - 1, position.y + 1, 0.0f, 1.0f)),
		};

		XMFLOAT4 color(0.0f, 1.0f, 1.0f, 1.0f); // temp color to test rendering

		uint32_t vertexCount = ARRAYSIZE(vertices);
		D3D11_MAPPED_SUBRESOURCE mappedSubResource;
		direct3DDeviceContext->Map(mVertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, NULL, &mappedSubResource);
		memcpy(mappedSubResource.pData, vertices, sizeof(VertexPosition) * vertexCount);
		direct3DDeviceContext->Unmap(mVertexBuffer.Get(), 0);
		direct3DDeviceContext->UpdateSubresource(mPSCBufferPerObject.Get(), 0, nullptr, &color, 0, 0);
		direct3DDeviceContext->DrawIndexed(mIndexCount, 0, 0);
	}

	void Player::HandleCollisions()
	{
		auto powerupManager = PowerupManager::GetInstance();

		// Handle cherry collisions
		if (powerupManager->GetCherryPosition() == mPosition)
		{
			powerupManager->RespawnCherry();
			IncreaseTail();
		}

		// Handle coin collisions
		if (powerupManager->GetCoinPosition() == mPosition)
		{
			powerupManager->RespawnCoin();
			IncreaseTail();
		}

		// Handle boundary collisions
		if (mPosition.x < (-27.0f * BodySize))
		{
			mAlive = false;
		}
		if (mPosition.x >= (27.0f * BodySize))
		{
			mAlive = false;
		}
		if (mPosition.y < (-13.0f * BodySize))
		{
			mAlive = false;
		}
		if (mPosition.y >= (13.0f * BodySize))
		{
			mAlive = false;
		}

		// Handle body collisions
		if (mTail.size() > 0)
		{
			for (uint32_t i = 1; i < mTail.size(); i++)
			{
				if (mPosition == mTail[i])
				{	// Player is kill
					mAlive = false;
				}
			}
		}
	}

}
