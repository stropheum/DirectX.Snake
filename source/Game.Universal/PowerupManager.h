#pragma once

namespace DirectXGame
{
	class PowerupManager final : public DX::DrawableGameComponent
	{
	public:
		PowerupManager(const std::shared_ptr<DX::DeviceResources>& deviceResources, const std::shared_ptr<DX::Camera>& camera);
		static std::shared_ptr<PowerupManager> Init(const std::shared_ptr<DX::DeviceResources>& deviceResources, const std::shared_ptr<DX::Camera>& camera);
		static std::shared_ptr<PowerupManager> GetInstance();

		virtual void CreateDeviceDependentResources() override;
		virtual void ReleaseDeviceDependentResources() override;
		virtual void Render(const DX::StepTimer& timer) override;
		void RenderCherry();
		void RenderCoin();
		Vector2f GetCherryPosition();
		Vector2f GetCoinPosition();
		void RespawnCherry();
		void RespawnCoin();

	private:
		static std::shared_ptr<PowerupManager> sInstance;
		
		// Constants
		const uint32_t BodySize = 25;

		// Private fields
		Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVSCBufferPerObject;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mPSCBufferPerObject;
		std::uint32_t mIndexCount;
		bool mLoadingComplete;

		Vector2f mCherryPosition;
		Vector2f mCoinPosition;
	};
}

