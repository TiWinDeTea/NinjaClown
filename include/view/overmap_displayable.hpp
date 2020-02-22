#ifndef NINJACLOWN_VIEW_OVERMAP_DISPLAYABLE_HPP
#define NINJACLOWN_VIEW_OVERMAP_DISPLAYABLE_HPP

namespace view {
class viewer;

// to display objects (buttons & cie) and mobs
// basically anything that is over the map
// can be ordered by Y coordinate
class overmap_displayable_interface {
public:
	// prints the displayable in a view
	virtual void vprint(view::viewer&) const = 0;

	// returns true if the displayable is currently hovered by the mouse
	virtual bool vis_hovered(view::viewer&) const noexcept = 0;

	bool operator<(const overmap_displayable_interface& other) const noexcept {
		return p_posy < other.p_posy;
	}

	virtual ~overmap_displayable_interface() = default;

protected:
	float p_posx{};
	float p_posy{};
};
}  // namespace view

#endif //NINJACLOWN_VIEW_OVERMAP_DISPLAYABLE_HPP
