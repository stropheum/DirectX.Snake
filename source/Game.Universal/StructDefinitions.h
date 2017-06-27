#pragma once

#pragma region Vector2f

typedef struct Vector2f
{
public:

	float x;
	float y;

	Vector2f():
		x(0.0f), y(0.0f)
	{
	}

	Vector2f(float _x, float _y):
		x(_x), y(_y)
	{
	}

	Vector2f(int _x, int _y):
		x(static_cast<float>(_x)), y(static_cast<float>(_y))
	{
	}

	Vector2f(float _x, int _y):
		x(_x), y(static_cast<float>(_y))
	{
	}

	Vector2f(int _x, float _y):
		x(static_cast<float>(_x)), y(_y)
	{
	}

	Vector2f& operator=(const Vector2f& rhs)
	{
		x = rhs.x;
		y = rhs.y;
		return (*this);
	}

	Vector2f operator+(const Vector2f& rhs)
	{
		return Vector2f(x + rhs.x, y + rhs.y);
	}

	Vector2f operator-(const Vector2f& rhs)
	{
		return Vector2f(x - rhs.x, y - rhs.y);
	}

	Vector2f& operator+=(const Vector2f& rhs)
	{
		x += rhs.x;
		y += rhs.y;
		return (*this);
	}

	Vector2f& operator-=(const Vector2f& rhs)
	{
		x -= rhs.x;
		y -= rhs.y;
		return (*this);
	}

	bool operator==(const Vector2f& rhs)
	{
		return (x == rhs.x && y == rhs.y);
	}

} Vector2f;

#pragma endregion