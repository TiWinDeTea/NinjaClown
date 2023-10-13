#ifndef NINJACLOWN_VIEW_ANIMATION_HPP
#define NINJACLOWN_VIEW_ANIMATION_HPP

#include <cassert>
#include <vector>

#include <SFML/Graphics/Sprite.hpp>

namespace view {
class map_viewer;

class animation {
public:
	static constexpr unsigned int SINGLE_IMAGE_DURATION = 500;

	void add_frame(sf::Sprite &&sprite) {
		m_frames.emplace_back(std::move(sprite));
	}

	void print(map_viewer &viewer, float posx, float posy) const noexcept;

	void highlight(map_viewer &viewer, float posx, float posy) const noexcept;

	/**
	 * Returns the sprite that should currently be displayed
	 */
	const sf::Sprite& current_sprite() const noexcept;

	friend class shifted_animation;

private:
	mutable std::vector<sf::Sprite> m_frames;
};

class shifted_animation {
public:
	static constexpr unsigned int SINGLE_IMAGE_DURATION = animation::SINGLE_IMAGE_DURATION;

	shifted_animation() noexcept(noexcept(std::vector<sf::Sprite>{})) = default;
	shifted_animation(const shifted_animation &)                      = default;
	shifted_animation(shifted_animation &&) noexcept                  = default;
	explicit shifted_animation(animation &&o) noexcept
	    : m_frames{std::move(o.m_frames)} { }
	~shifted_animation() = default;

	shifted_animation &operator=(shifted_animation &&) noexcept = default;
	shifted_animation &operator=(const shifted_animation &)     = default;

	bool is_hovered(map_viewer &viewer) const noexcept;

	void add_frame(sf::Sprite &&sprite) {
		m_frames.emplace_back(std::move(sprite));
	}

	void set_shift(float xshift, float yshift) {
		m_xshift = xshift;
		m_yshift = yshift;
	}

	void print(map_viewer &viewer, float posx, float posy) const noexcept;

	/**
	 * Returns the sprite that should currently be displayed
	 * @param frame_count total number of frames that have been displayed by the program
	 */
	const sf::Sprite& current_sprite() const noexcept;

	bool empty() const noexcept {
		return m_frames.empty();
	}

private:
	mutable std::vector<sf::Sprite> m_frames;
	float m_xshift{};
	float m_yshift{};
};
} // namespace view
#endif //NINJACLOWN_VIEW_ANIMATION_HPP
