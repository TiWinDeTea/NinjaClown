#ifndef NINJACLOWN_VIEW_OVERMAP_DISPLAYABLE_HPP
#define NINJACLOWN_VIEW_OVERMAP_DISPLAYABLE_HPP

namespace view {
class map_viewer;

// to display objects (buttons & cie) and mobs
// basically anything that is over the map
// can be ordered by Y coordinate
class overmap_displayable_interface {
public:

	void print(map_viewer& v) const {
		if (!m_hidden) {
			vprint(v);
		}
	}

	bool is_hovered(const map_viewer& v) const noexcept {
		return !m_hidden && vis_hovered(v);
	}

	bool operator<(const overmap_displayable_interface& other) const noexcept {
		return p_posy < other.p_posy;
	}

	virtual ~overmap_displayable_interface() = default;

	void hide() noexcept {
		m_hidden = true;
	}

	void reveal() noexcept {
		m_hidden = false;
	}

protected:
    // prints the displayable in a view
    virtual void vprint(map_viewer&) const = 0;

    // returns true if the displayable is currently hovered by the mouse
    virtual bool vis_hovered(const map_viewer&) const noexcept = 0;

	float p_posx{};
	float p_posy{};

private:
    bool m_hidden{false};
};
}  // namespace view

#endif //NINJACLOWN_VIEW_OVERMAP_DISPLAYABLE_HPP
