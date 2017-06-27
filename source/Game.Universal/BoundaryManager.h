#pragma once

namespace DirectXGame
{
	class BoundaryManager final : public DX::DrawableGameComponent
	{
	public:
		BoundaryManager(const std::shared_ptr<DX::DeviceResources>& deviceResources, const std::shared_ptr<DX::Camera>& camera);
		virtual void CreateDeviceDependentResources() override;
		virtual void ReleaseDeviceDependentResources() override;
		virtual void Render(const DX::StepTimer& timer) override;
	private:
		// Constants
		const uint32_t BodySize = 25;

		// Private methdos
		void RenderTop();
		void RenderBottom();
		void RenderLeft();
		void RenderRight();

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
	};
}

