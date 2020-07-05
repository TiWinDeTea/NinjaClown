#ifndef NINJACLOWN_VIEW_EVENT_INSPECTOR_HPP
#define NINJACLOWN_VIEW_EVENT_INSPECTOR_HPP

namespace sf {
struct Event;
}

namespace view {
class viewer;
struct viewer_display_state;

void inspect_event(view::viewer&, const sf::Event& event, viewer_display_state& state);
}

#endif //NINJACLOWN_VIEW_EVENT_INSPECTOR_HPP
