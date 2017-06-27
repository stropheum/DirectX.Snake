#pragma once
#include "StructDefinitions.h"

namespace DirectXGame
{
	class SixteenSegmentManager final : public DX::DrawableGameComponent
	{
	public:
		static const uint16_t ZERO =	/*0100 0100 1111 1111*/ 0x44FF;
		static const uint16_t ONE =		/*0010 0010 0000 0000*/ 0x2200;
		static const uint16_t TWO =		/*1000 1000 0111 0111*/ 0x8877;
		static const uint16_t THREE =	/*1000 1000 0011 1111*/ 0x883F;
		static const uint16_t FOUR =	/*1000 1000 1000 1100*/ 0x888C;
		static const uint16_t FIVE =	/*1000 1000 1011 1011*/ 0x88BB;
		static const uint16_t SIX =		/*1000 1000 1111 1011*/ 0x88FB;
		static const uint16_t SEVEN =	/*0000 0000 0000 1111*/ 0x000F;
		static const uint16_t EIGHT =	/*1000 1000 1111 1111*/ 0x88FF;
		static const uint16_t NINE =	/*1000 1000 1000 1111*/ 0x888F;

		static const uint16_t A =		/*1000 1000 1100 1111*/ 0x88CF;
		static const uint16_t B =		/*1001 0100 1111 0011*/ 0x94F3;
		static const uint16_t C =		/*0000 0000 1111 0011*/ 0x00F3;
		static const uint16_t D =		/*0101 0001 1100 0000*/ 0x41C0;
		static const uint16_t E =		/*1000 0000 1111 0011*/ 0x80F3;
		static const uint16_t F =		/*1000 0000 1100 0011*/ 0x80C3;
		static const uint16_t G =		/*0000 1000 1111 1011*/ 0x08FB;
		static const uint16_t H =		/*1000 1000 1100 1100*/ 0x88CC;
		static const uint16_t I =		/*0010 0010 0011 0011*/ 0x2233;
		static const uint16_t J =		/*0000 0000 0011 1100*/ 0x003C;
		static const uint16_t K =		/*1001 0100 1100 0000*/ 0x94C0;
		static const uint16_t L =		/*0000 0000 1111 0000*/ 0x00F0;
		static const uint16_t M =		/*0000 0101 1100 1100*/ 0x05CC;
		static const uint16_t N =		/*0001 0001 1100 1100*/ 0x11CC;
		static const uint16_t O =		/*0000 0000 1111 1111*/ 0x00FF;
		static const uint16_t P =		/*1000 1000 1100 0111*/ 0x88C7;
		static const uint16_t Q =		/*0001 0000 1111 1111*/ 0x10FF;
		static const uint16_t R =		/*1001 1000 1100 0111*/ 0x98C7;
		static const uint16_t S =		/*1000 1000 1011 1011*/ 0x88BB;
		static const uint16_t T =		/*0010 0010 0000 0011*/ 0x2203;
		static const uint16_t U =		/*0000 0000 1111 1100*/ 0x00FC;
		static const uint16_t V =		/*0100 0100 1100 0000*/ 0x44C0;
		static const uint16_t W =		/*0101 0000 1100 1100*/ 0x50CC;
		static const uint16_t X =		/*0101 0101 0000 0000*/ 0x5500;
		static const uint16_t Y =		/*0010 0101 0000 0000*/ 0x2500;
		static const uint16_t Z =		/*0100 0100 0011 0011*/ 0x4433;

		SixteenSegmentManager(const std::shared_ptr<DX::DeviceResources>& deviceResources, const std::shared_ptr<DX::Camera>& camera);
		~SixteenSegmentManager();

		static std::shared_ptr<SixteenSegmentManager> Init(const std::shared_ptr<DX::DeviceResources>& deviceResources, const std::shared_ptr<DX::Camera>& camera);
		static std::shared_ptr<SixteenSegmentManager> GetInstance();

		void InitializeFunctionPointers();

		virtual void CreateDeviceDependentResources() override;
		virtual void ReleaseDeviceDependentResources() override;
		virtual void Render(const DX::StepTimer& timer) override;

		void Render(const DX::StepTimer& timer, uint16_t value, float x, float y);
		void Render(const DX::StepTimer& timer, char c, float x, float y);
		void DisplayString(const DX::StepTimer& timer, char* word, float x, float y);

	protected:
		static std::shared_ptr<SixteenSegmentManager> sInstance;

		void (SixteenSegmentManager::* displaySegment[16])(bool on, float x, float y);

		void seg_0(bool on, float x, float y);
		void seg_1(bool on, float x, float y);
		void seg_2(bool on, float x, float y);
		void seg_3(bool on, float x, float y);
		void seg_4(bool on, float x, float y);
		void seg_5(bool on, float x, float y);
		void seg_6(bool on, float x, float y);
		void seg_7(bool on, float x, float y);
		void seg_8(bool on, float x, float y);
		void seg_9(bool on, float x, float y);
		void seg_A(bool on, float x, float y);
		void seg_B(bool on, float x, float y);
		void seg_C(bool on, float x, float y);
		void seg_D(bool on, float x, float y);
		void seg_E(bool on, float x, float y);
		void seg_F(bool on, float x, float y);

		// Private fields
		const uint8_t SEG_ON = 0x00FF;
		const uint8_t SEG_OFF = 0x0033;

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

