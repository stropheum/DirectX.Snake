#include "pch.h"
#include "KeyboardComponent.h"

using namespace std;
using namespace DirectX;

namespace DX
{
	unique_ptr<Keyboard> KeyboardComponent::sKeyboard(new DirectX::Keyboard);

	Keyboard* KeyboardComponent::Keyboard()
	{
		return sKeyboard.get();
	}

	KeyboardComponent::KeyboardComponent(const shared_ptr<DX::DeviceResources>& deviceResources) :
		GameComponent(deviceResources)
	{
		mCurrentState = sKeyboard->GetState();
		mLastState = mCurrentState;
	}

	const Keyboard::State& KeyboardComponent::CurrentState() const
	{
		return mCurrentState;
	}

	const Keyboard::State& KeyboardComponent::LastState() const
	{
		return mLastState;
	}

	void KeyboardComponent::Update(const StepTimer& timer)
	{
		UNREFERENCED_PARAMETER(timer);

		mLastState = mCurrentState;
		mCurrentState = sKeyboard->GetState();
	}

	bool KeyboardComponent::IsKeyUp(Keys key) const
	{
		return mCurrentState.IsKeyUp(static_cast<Keyboard::Keys>(key));
	}

	bool KeyboardComponent::IsKeyDown(Keys key) const
	{
		return mCurrentState.IsKeyDown(static_cast<Keyboard::Keys>(key));
	}

	bool KeyboardComponent::WasKeyUp(Keys key) const
	{
		return mLastState.IsKeyUp(static_cast<Keyboard::Keys>(key));
	}

	bool KeyboardComponent::WasKeyDown(Keys key) const
	{
		return mLastState.IsKeyDown(static_cast<Keyboard::Keys>(key));
	}

	bool KeyboardComponent::WasKeyPressedThisFrame(Keys key) const
	{
		return (IsKeyDown(key) && WasKeyUp(key));
	}

	bool KeyboardComponent::WasKeyReleasedThisFrame(Keys key) const
	{
		return (IsKeyUp(key) && WasKeyDown(key));
	}

	bool KeyboardComponent::IsKeyHeldDown(Keys key) const
	{
		return (IsKeyDown(key) && WasKeyDown(key));
	}
}