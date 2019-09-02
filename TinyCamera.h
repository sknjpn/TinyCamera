# pragma once

/* プリコンパイルでSiv3D.hppを読み込む
# include <Siv3D.hpp> // OpenSiv3D v0.2.6
*/

class BasicCamera
{
protected:
	Vec2	m_center = Scene::Center();
	double	m_magnification = 1.0;

public:
	BasicCamera() = default;

	BasicCamera(const Vec2& center, double magnification)
		: m_center(center)
		, m_magnification(magnification)
	{}

	[[nodiscard]] RectF getCameraRect() const { return RectF(Scene::Size() / m_magnification).setCenter(m_center); }
	[[nodiscard]] Mat3x2 getMat3x2() const { return Mat3x2::Translate(-m_center).scaled(m_magnification).translated(Scene::Size() * 0.5); }
	[[nodiscard]] Transformer2D createTransformer() const { return Transformer2D(getMat3x2(), true); }

	void		setCenter(const Vec2& center) { m_center = center; }
	void		setMagnification(double magnification) { m_magnification = magnification; }

	const Vec2& getCenter() const noexcept { return m_center; }
	double		getMagnification() const noexcept { return m_magnification; }
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
	double		m_targetMagnification = 1.0;

	void magnify()
	{
		const auto delta = 1.0 + m_magnifyingSensitivity * Mouse::Wheel();
		const auto cursorPos = (Cursor::PosF() - Scene::Size() * 0.5) / m_targetMagnification + m_targetCenter;

		m_targetMagnification /= delta;
		m_targetCenter = (m_targetCenter - cursorPos) * delta + cursorPos;
	}

	void move()
	{
		if (m_controls[0]()) { m_targetCenter.y -= m_movingSensitivity * Scene::Size().y / m_targetMagnification; }
		if (m_controls[1]()) { m_targetCenter.x -= m_movingSensitivity * Scene::Size().x / m_targetMagnification; }
		if (m_controls[2]()) { m_targetCenter.y += m_movingSensitivity * Scene::Size().y / m_targetMagnification; }
		if (m_controls[3]()) { m_targetCenter.x += m_movingSensitivity * Scene::Size().x / m_targetMagnification; }
	}

	void follow()
	{
		m_center = Math::Lerp(m_center, m_targetCenter, m_followingSpeed);
		m_magnification = 1.0 / Math::Lerp(1.0 / m_magnification, 1.0 / m_targetMagnification, m_followingSpeed);
	}

public:
	CursorCamera() = default;

	CursorCamera(const Vec2& targetCenter, double targetMagnification)
		: BasicCamera(targetCenter, targetMagnification)
		, m_targetCenter(targetCenter)
		, m_targetMagnification(targetMagnification)
	{}

	CursorCamera(const Vec2& targetCenter, double targetMagnification, double followingSpeed, double magnifyingSensitivity, double movingSensitivity)
		: BasicCamera(targetCenter, targetMagnification)
		, m_targetCenter(targetCenter)
		, m_targetMagnification(targetMagnification)
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
	void	setTargetMagnification(double targetMagnification) noexcept { m_targetMagnification = targetMagnification; }

	RectF	getTargetCameraRect() const { return RectF(Scene::Size() / m_targetMagnification).setCenter(m_targetCenter); }
};

class RestrictedCamera2D
	: public CursorCamera
{
	RectF	m_restrictedRect = Scene::Rect();
	double	m_minMagnification = 1.0;
	double	m_maxMagnification = 8.0;

	void	restrictMagnification()
	{
		auto min = Max({ m_minMagnification, Scene::Size().y / m_restrictedRect.h, Scene::Size().x / m_restrictedRect.w });
		auto max = m_maxMagnification;

		if (m_magnification < min) { m_magnification = min; }
		if (m_magnification > max) { m_magnification = max; }
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

	void	restrictTargetMagnification()
	{
		auto min = Max({ m_minMagnification, Scene::Size().y / m_restrictedRect.h, Scene::Size().x / m_restrictedRect.w });
		auto max = m_maxMagnification;

		if (m_targetMagnification < min) { m_targetMagnification = min; }
		if (m_targetMagnification > max) { m_targetMagnification = max; }
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

	RestrictedCamera2D(const Vec2& targetCenter, double targetMagnification)
		: CursorCamera(targetCenter, targetMagnification)
	{
		restrictMagnification();
		restrictRect();
		restrictTargetMagnification();
		restrictTargetRect();
	}

	RestrictedCamera2D(const RectF restrictedRect, double minMagnification, double maxMagnification)
		: m_restrictedRect(restrictedRect)
		, m_minMagnification(minMagnification)
		, m_maxMagnification(maxMagnification)
	{
		restrictMagnification();
		restrictRect();
		restrictTargetMagnification();
		restrictTargetRect();
	}

	RestrictedCamera2D(const Vec2& targetCenter, double targetMagnification, const RectF restrictedRect, double minMagnification, double maxMagnification)
		: CursorCamera(targetCenter, targetMagnification)
		, m_restrictedRect(restrictedRect)
		, m_minMagnification(minMagnification)
		, m_maxMagnification(maxMagnification)
	{
		restrictMagnification();
		restrictRect();
		restrictTargetMagnification();
		restrictTargetRect();
	}

	void update()
	{
		magnify();

		restrictTargetMagnification();

		move();

		restrictTargetRect();

		follow();
	}

	void	setRestrictedRect(RectF restrictedRect)
	{
		m_restrictedRect = restrictedRect;

		restrictMagnification();
		restrictRect();
		restrictTargetMagnification();
		restrictTargetRect();
	}

	void	setMaxMagnification(double maxMagnification)
	{
		m_maxMagnification = maxMagnification;

		restrictMagnification();
		restrictRect();
		restrictTargetMagnification();
		restrictTargetRect();
	}

	void	setMinMagnification(double minMagnification)
	{
		m_minMagnification = minMagnification;

		restrictMagnification();
		restrictRect();
		restrictTargetMagnification();
		restrictTargetRect();
	}

	[[nodiscard]] const RectF& getRestrictedRect() const noexcept { return m_restrictedRect; }
	[[nodiscard]] double		getMinMagnification() const noexcept { return m_minMagnification; }
	[[nodiscard]] double		getMaxMagnification() const noexcept { return m_maxMagnification; }
};