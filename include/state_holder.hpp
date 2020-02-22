#ifndef NINJACLOWN_STATE_HOLDER_HPP
#define NINJACLOWN_STATE_HOLDER_HPP

#include <filesystem>
#include <memory>

#include <imterm/terminal.hpp>

#include "adapter/adapter.hpp"
#include "model/model.hpp"
#include "terminal_commands.hpp"
#include "utils/resource_manager.hpp"
#include "view/viewer.hpp"

namespace state {

template <typename>
class access;

class holder {
	std::shared_ptr<terminal_commands> m_command_manager;

public:
	explicit holder(const std::filesystem::path &config) noexcept;

	void run() noexcept;

	void wait() noexcept;

	utils::resource_manager resources;

private:
	ImTerm::terminal<terminal_commands> m_terminal;

	model::model m_model;
	view::viewer m_view;

	adapter::adapter m_adapter;

	friend terminal_commands;
	friend access<view::viewer>;
	friend access<adapter::adapter>;
	friend access<model::model>;
};

template <typename>
class access {};

template <>
class access<view::viewer> {
	static ImTerm::terminal<terminal_commands> &terminal(holder &holder) noexcept {
		return holder.m_terminal;
	}

	static adapter::adapter &adapter(holder &holder) noexcept {
		return holder.m_adapter;
	}

	friend view::viewer;
};

template <>
class access<model::model> {
	static adapter::adapter &adapter(holder &holder) noexcept {
		return holder.m_adapter;
	}

	friend model::model;
};

template <>
class access<adapter::adapter> {
	static model::model &model(holder &holder) noexcept {
		return holder.m_model;
	}

	static view::viewer &view(holder &holder) noexcept {
		return holder.m_view;
	}

	friend adapter::adapter;
};

} // namespace state

#endif //NINJACLOWN_STATE_HOLDER_HPP
