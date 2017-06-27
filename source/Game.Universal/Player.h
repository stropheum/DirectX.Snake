#pragma once
#include <Vector>
#include "StructDefinitions.h"

namespace DirectXGame
{
	class Player final : public DX::DrawableGameComponent
	{
	public:

		enum Direction
		{
			Stop, Up, Down, Left, Right
		};

		Player(const std::shared_ptr<DX::DeviceResources>& deviceResources, const std::shared_ptr<DX::Camera>& camera);

		virtual void CreateDeviceDependentResources() override;
		virtual void ReleaseDeviceDependentResources() override;
		virtual void Render(const DX::StepTimer& timer) override;

		void Move(Direction direction);
		void UpdateTail();
		void IncreaseTail();
		uint32_t GetTailSize();
		Vector2f GetTailAt(uint32_t index);
		void HandleCollisions();
		bool Alive();
		void Kill();
		void Respawn();

	private:

		// Constants
		const uint32_t MaxLength = 1296;
		const uint32_t BodySize = 25;

		// Private methods
		void RenderSquare(Vector2f position);

		// Private fields
		Microsoft::WRL::ComPtr<ID3D11VertexShader> mVertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader> mPixelShader;
		Microsoft::WRL::ComPtr<ID3D11InputLayout> mInputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mIndexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mVSCBufferPerObject;
		Microsoft::WRL::ComPtr<ID3D11Buffer> mPSCBufferPerObject;

		Vector2f mPosition;
		std::vector<Vector2f> mTail;
		Vector2f mVelocity;
		std::uint32_t mIndexCount;
		bool mLoadingComplete;
		double mTimeSinceUpdate;
		Direction mDirection;
		bool mAlive;
		float mGameSpeed;
		bool mDirectionLocked;
	};


}
