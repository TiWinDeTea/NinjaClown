struct bot_api {
	void (*log)(const char*);
	void (*go_right)();
};

static struct bot_api* BOT;

void bot_init(struct bot_api* api) {
	BOT = api;
}

void bot_think() {
	BOT->log("hello from test bot!");
	BOT->go_right();
}
