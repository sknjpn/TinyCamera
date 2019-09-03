# pragma once

/* プリコンパイルでSiv3D.hppを読み込む
# include <Siv3D.hpp> // OpenSiv3D v0.2.6
*/

class BasicCamera
{
protected:
	Vec2	m_center = Scene::Center();
	double	m_scale= 1.0;

public:
	BasicCamera() = default;

	BasicCamera(const Vec2& center, double scale)
		: m_center(center)
		, m_scale(scale)
	{}

	[[nodiscard]] RectF getCameraRect() const { return RectF(Scene::Size() / m_scale).setCenter(m_center); }
	[[nodiscard]] Mat3x2 getMat3x2() const { return Mat3x2::Translate(-m_center).scaled(m_scale).translated(Scene::Size() * 0.5); }
	[[nodiscard]] Transformer2D createTransformer() const { return Transformer2D(getMat3x2(), true); }

	void		setCenter(const Vec2& center) { m_center = center; }
	void		setScale(double scale) { m_scale= scale; }

	const Vec2& getCenter() const noexcept { return m_center; }
	double		getScale() const noexcept { return m_scale; }
};

class CursorCamera
	: public BasicCamera
{
	double		m_followingSpeed = 0.25;
	double		m_magnifyingSensitivity = 0.1;
	double		m_movingSensitivity = 0.02;

	std::array<std::function<bool()>, 4> m_controls =
	{
		[]() { return KeyW.pressed() || Cursor::Pos().y <= 0; },
		[]() { return KeyA.pressed() || Cursor::Pos().x <= 0; },
		[]() { return KeyS.pressed() || Cursor::Pos().y >= Scene::Height() - 1; },
		[]() { return KeyD.pressed() || Cursor::Pos().x >= Scene::Width() - 1; },
	};

protected:
	Vec2		m_targetCenter = Scene::Size() * 0.5;
	double		m_targetScale= 1.0;

	void magnify()
	{
		const auto delta = 1.0 + m_magnifyingSensitivity * Mouse::Wheel();
		const auto cursorPos = (Cursor::PosF() - Scene::Size() * 0.5) / m_targetScale+ m_targetCenter;

		m_targetScale/= delta;
		m_targetCenter = (m_targetCenter - cursorPos) * delta + cursorPos;
	}

	void move()
	{
		if (m_controls[0]()) { m_targetCenter.y -= m_movingSensitivity * Scene::Size().y / m_targetScale; }
		if (m_controls[1]()) { m_targetCenter.x -= m_movingSensitivity * Scene::Size().x / m_targetScale; }
		if (m_controls[2]()) { m_targetCenter.y += m_movingSensitivity * Scene::Size().y / m_targetScale; }
		if (m_controls[3]()) { m_targetCenter.x += m_movingSensitivity * Scene::Size().x / m_targetScale; }
	}

	void follow()
	{
		m_center = Math::Lerp(m_center, m_targetCenter, m_followingSpeed);
		m_scale= 1.0 / Math::Lerp(1.0 / m_scale, 1.0 / m_targetScale, m_followingSpeed);
	}

public:
	CursorCamera() = default;

	CursorCamera(const Vec2& targetCenter, double targetScale)
		: BasicCamera(targetCenter, targetScale)
		, m_targetCenter(targetCenter)
		, m_targetScale(targetScale)
	{}

	CursorCamera(const Vec2& targetCenter, double targetScale, double followingSpeed, double magnifyingSensitivity, double movingSensitivity)
		: BasicCamera(targetCenter, targetScale)
		, m_targetCenter(targetCenter)
		, m_targetScale(targetScale)
		, m_followingSpeed(followingSpeed)
		, m_magnifyingSensitivity(magnifyingSensitivity)
		, m_movingSensitivity(movingSensitivity)
	{}

	void update()
	{
		magnify();

		move();

		follow();
	}

	void	setFollowingSpeed(double followingSpeed) noexcept { m_followingSpeed = followingSpeed; }
	void	setMagnifyingSensitivity(double magnifyingSensitivity) noexcept { m_magnifyingSensitivity = magnifyingSensitivity; }
	void	setMovingSensitivity(double movingSensitivity) noexcept { m_movingSensitivity = movingSensitivity; }
	void	setControls(const std::array<std::function<bool()>, 4> & controls) noexcept { m_controls = controls; }

	void	setTargetCenter(const Vec2& targetCenter) noexcept { m_targetCenter = targetCenter; }
	void	setTargetScale(double targetScale) noexcept { m_targetScale= targetScale; }

	RectF	getTargetCameraRect() const { return RectF(Scene::Size() / m_targetScale).setCenter(m_targetCenter); }
};

class RestrictedCamera2D
	: public CursorCamera
{
	RectF	m_restrictedRect = Scene::Rect();
	double	m_minScale= 1.0;
	double	m_maxScale= 8.0;

	void	restrictScale()
	{
		auto min = Max({ m_minScale, Scene::Size().y / m_restrictedRect.h, Scene::Size().x / m_restrictedRect.w });
		auto max = m_maxScale;

		if (m_scale< min) { m_scale= min; }
		if (m_scale> max) { m_scale= max; }
	}

	void	restrictRect()
	{
		const auto cameraRect = getCameraRect();

		if (m_restrictedRect.contains(cameraRect)) { return; }

		const auto tl = m_restrictedRect.tl() - cameraRect.tl();
		const auto br = m_restrictedRect.br() - cameraRect.br();

		if (tl.x > 0) { m_center.moveBy(tl.x, 0); }
		if (tl.y > 0) { m_center.moveBy(0, tl.y); }
		if (br.x < 0) { m_center.moveBy(br.x, 0); }
		if (br.y < 0) { m_center.moveBy(0, br.y); }
	}

	void	restrictTargetScale()
	{
		auto min = Max({ m_minScale, Scene::Size().y / m_restrictedRect.h, Scene::Size().x / m_restrictedRect.w });
		auto max = m_maxScale;

		if (m_targetScale< min) { m_targetScale= min; }
		if (m_targetScale> max) { m_targetScale= max; }
	}

	void	restrictTargetRect()
	{
		const auto targetCameraRect = getTargetCameraRect();

		if (m_restrictedRect.contains(targetCameraRect)) { return; }

		const auto tl = m_restrictedRect.tl() - targetCameraRect.tl();
		const auto br = m_restrictedRect.br() - targetCameraRect.br();

		if (tl.x > 0) { m_targetCenter.moveBy(tl.x, 0); }
		if (tl.y > 0) { m_targetCenter.moveBy(0, tl.y); }
		if (br.x < 0) { m_targetCenter.moveBy(br.x, 0); }
		if (br.y < 0) { m_targetCenter.moveBy(0, br.y); }
	}

public:
	RestrictedCamera2D() = default;

	RestrictedCamera2D(const Vec2& targetCenter, double targetScale)
		: CursorCamera(targetCenter, targetScale)
	{
		restrictScale();
		restrictRect();
		restrictTargetScale();
		restrictTargetRect();
	}

	RestrictedCamera2D(const RectF restrictedRect, double minScale, double maxScale)
		: m_restrictedRect(restrictedRect)
		, m_minScale(minScale)
		, m_maxScale(maxScale)
	{
		restrictScale();
		restrictRect();
		restrictTargetScale();
		restrictTargetRect();
	}

	RestrictedCamera2D(const Vec2& targetCenter, double targetScale, const RectF restrictedRect, double minScale, double maxScale)
		: CursorCamera(targetCenter, targetScale)
		, m_restrictedRect(restrictedRect)
		, m_minScale(minScale)
		, m_maxScale(maxScale)
	{
		restrictScale();
		restrictRect();
		restrictTargetScale();
		restrictTargetRect();
	}

	void update()
	{
		magnify();

		restrictTargetScale();

		move();

		restrictTargetRect();

		follow();
	}

	void	setRestrictedRect(RectF restrictedRect)
	{
		m_restrictedRect = restrictedRect;

		restrictScale();
		restrictRect();
		restrictTargetScale();
		restrictTargetRect();
	}

	void	setMaxScale(double maxScale)
	{
		m_maxScale= maxScale;

		restrictScale();
		restrictRect();
		restrictTargetScale();
		restrictTargetRect();
	}

	void	setMinScale(double minScale)
	{
		m_minScale= minScale;

		restrictScale();
		restrictRect();
		restrictTargetScale();
		restrictTargetRect();
	}

	[[nodiscard]] const RectF& getRestrictedRect() const noexcept { return m_restrictedRect; }
	[[nodiscard]] double		getMinScale() const noexcept { return m_minScale; }
	[[nodiscard]] double		getMaxScale() const noexcept { return m_maxScale; }
};