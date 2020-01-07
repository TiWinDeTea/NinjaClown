#ifndef NINJACLOWN_ADAPTER_ADAPTER_HPP
#define NINJACLOWN_ADAPTER_ADAPTER_HPP

#include <filesystem>
#include <unordered_map>

#include "model/cell.hpp"

namespace adapter {
struct model_handle {
    explicit model_handle() noexcept = default;
    explicit constexpr model_handle(size_t handle_) noexcept : handle{handle_} {}
    constexpr bool operator==(const model_handle& other) const noexcept {
        return other.handle == handle;
    }
    size_t handle{};
};

struct view_handle {
    explicit view_handle() noexcept = default;
    explicit constexpr view_handle(bool is_mob_, size_t handle_) noexcept : is_mob{is_mob_}, handle{handle_} {}
    constexpr bool operator==(const view_handle& other) const noexcept {
        return other.is_mob == is_mob && other.handle == handle;
    }
    bool is_mob{};
    size_t handle{};
};

struct model_hhash : private std::hash<size_t> {
    size_t operator()(const model_handle& h) const noexcept {
        return std::hash<size_t>::operator()(h.handle);
    }
};
struct view_hhash {
    size_t operator()(const view_handle& h) const noexcept;
};

struct draw_request {
    enum class request_type_t {
        hitbox_highlight,
        tile_highlight
    };
    struct coords_t {
        size_t x, y;
    };
    struct hitbox_t {
        float x, y;
        float width, height;
    };

    request_type_t request_type;
    union {
        coords_t coords;
        hitbox_t hitbox;
	} data;
};

class adapter {
public:

	bool load_map(const std::filesystem::path &path) noexcept;

	void update_map(size_t y, size_t x, model::cell_type new_cell) noexcept;

	void move_entity(model_handle entity, float new_x, float new_y) noexcept;

	void rotate_entity(model_handle entity, float new_rad) noexcept;

    [[nodiscard]] std::optional<draw_request> tooltip_for(view_handle entity) noexcept;
private:
    std::unordered_map<model_handle, view_handle, model_hhash> m_model2view;
    std::unordered_map<view_handle, model_handle, view_hhash> m_view2model;
};
}

#endif //NINJACLOWN_ADAPTER_ADAPTER_HPP
