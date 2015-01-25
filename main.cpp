#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <list>
#include <sstream>
#include <vector>

#include <SFML/Graphics.hpp>

unsigned int winw;
unsigned int winh;
const unsigned int game_step = 16;
float game_time;
sf::Vector2f gravity;

using std::rand;

bool load(sf::Texture& tex, const std::string& file)
{
	if (!tex.loadFromFile(file))
	{
		std::cerr << "Failed to load texture " << file << std::endl;
		return false;
	}
	return true;
}

float rad2deg(float rad)
{
	return (rad * 180.f) / M_PI;
}

float dot(const sf::Vector2f& v1, const sf::Vector2f& v2)
{
	return v1.x * v2.x + v1.y * v2.y;
}

float dist2(const sf::Vector2f& p1, const sf::Vector2f& p2)
{
	return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
}

float dist(const sf::Vector2f& p1, const sf::Vector2f& p2)
{
	return sqrtf(dist2(p1, p2));
}

float norm2(const sf::Vector2f& v)
{
	return v.x * v.x + v.y * v.y;
}

float norm(const sf::Vector2f& v)
{
	return sqrtf(norm2(v));
}

sf::Vector2f normv(const sf::Vector2f& v)
{
	return v / norm(v);
}

class Grappable
{
protected:
	sf::Vector2f position;
	sf::Vector2f velocity;
public:
	Grappable(float x, float y)
		: position {x, y}, velocity {0.f, 0.f}
	{}

	Grappable(const sf::Vector2f& v)
		: position {v}
	{}

	inline const sf::Vector2f& pos() const
	{
		return position;
	}

	inline const sf::Vector2f& vel() const
	{
		return velocity;
	}
};

class Point : public Grappable
{
	sf::Sprite sprite;
public:
	Point(float x, float y, const sf::Texture& texture)
		: Grappable {x, y}, sprite {texture}
	{
		auto s = texture.getSize();
		sprite.setOrigin(s.x / 2.f, s.y / 2.f);
		sprite.setScale(4.f, 4.f);
		sprite.setPosition(position);
	}

	void draw_on(sf::RenderTexture& render_target)
	{
		render_target.draw(sprite);
	}
};

class Swinger : public Grappable
{
	sf::Sprite avatar;
	sf::Sprite reticle;
	sf::Sprite aimbox;
	sf::Sprite rope;

	Grappable* grapple_target = nullptr;
	Grappable* nearest = nullptr;
	// 0 = not grappling, 1 = moving toward point, 2 = swingin'
	int grappling = 0;

	float max_grap_dist = 200.f;
	float max_grap_dist2;
	float grap_dist;

	float pull_speed_factor = 0.004f;
	float min_pull_speed = 0.04f;

	float aiming = false;

	float max_target_dist = 400.f;
	float max_target_dist2;

	// velocity in the reference frame of swinging
	float swing_vel = 0.f;
	float starting_swing_vel = 0.004f;

	int need_center = 0;

	sf::Vector2f last_target_pos;

	int lives = 3;

	float half_height;
	float half_width;

	int index;

	sf::Text textbox;
	sf::RectangleShape textboxbox;
	sf::Clock texttimer;
	float texttime = -1.f;
	sf::FloatRect textbounds;
	sf::ConvexShape textarrow;

	std::string name;

	bool dead = false;
	sf::Clock dead_timer;
public:
	Swinger(int i, const std::string& nm, const sf::Font& font, float x, const sf::Color& color, const sf::Texture& avatar_tex, const sf::Texture& reticle_tex,  const sf::Texture& aimbox_tex, const sf::Texture& rope_tex)
		: Grappable {x, 0.f}, name {nm}, avatar {avatar_tex}, reticle {reticle_tex}, aimbox {aimbox_tex}, rope {rope_tex}
	{
		index = i;
		auto s = avatar_tex.getSize();
		float scale = 4.f;
		half_height = s.y * scale / 2.f;
		half_width = s.x * scale / 2.f;
		position.y = winh - half_height;

		avatar.setOrigin(s.x / 2.f, s.y / 2.f);
		avatar.setScale(scale, scale);
		avatar.setColor(color);

		s = reticle_tex.getSize();
		reticle.setOrigin(s.x / 2.f, s.y / 2.f);
		reticle.setScale(scale, scale);
		reticle.setColor(color);

		s = aimbox_tex.getSize();
		aimbox.setOrigin(s.x / -2.f, s.y / 2.f);
		aimbox.setScale(scale, scale);
		aimbox.setColor(sf::Color {color.r, color.g, color.b, 50});

		s = rope_tex.getSize();
		rope.setOrigin(0.f, s.y / 2.f);
		rope.setColor(sf::Color {(sf::Uint8)(color.r / 3), (sf::Uint8)(color.g / 3), (sf::Uint8)(color.b / 3)});

		max_grap_dist2 = max_grap_dist * max_grap_dist;
		max_target_dist2 = max_target_dist * max_target_dist;

		textbox.setFont(font);
		textbox.setCharacterSize(20);
		textbox.setColor(sf::Color::Black);
		textboxbox.setFillColor(sf::Color::White);
		textboxbox.setOrigin(5.f, 5.f);
		textarrow.setPointCount(3);
		textarrow.setPoint(0, sf::Vector2f {0.f, 10.f});
		textarrow.setPoint(1, sf::Vector2f {1.f, 0.f});
		textarrow.setPoint(2, sf::Vector2f {0.f, -10.f});
		textarrow.setFillColor(sf::Color::White);
		textarrow.setOrigin(0.f, 5.f);
	}

	const std::string& get_name()
	{
		return name;
	}

	void say(const std::string& txt, float time)
	{
		textbox.setString(txt);
		textbounds = textbox.getLocalBounds();
		textboxbox.setSize(sf::Vector2f{textbounds.width + 20.f, textbounds.height + 20.f});
		texttimer.restart();
		texttime = time;
	}

	void lament(const std::string nm)
	{
		int r = rand() % 5;
		std::string l;
		switch (r)
		{
			case 0:
				l = nm + "!? " + nm + "!!!!";
				break;
			case 1:
				l = nm + ", I'LL NEVER LET GO!";
				break;
			case 2:
				l = nm + "! WHY????";
				break;
			case 3:
				l = nm + ", I WILL TELL YOUR FAMILY THAT YOU LOVE THEM!";
				break;
			case 4:
				l = "HE WAS ONLY TWO DAYS FROM RETIREMENT!";
				break;
		}
		say(l, 3);
	}

	int get_lives() const
	{
		return lives;
	}

	float get_half_height() const
	{
		return half_height;
	}

	void die()
	{
		--lives;
		let_go();
		dead = true;
		dead_timer.restart();
	}

	bool is_dead() const
	{
		return dead;
	}

	bool need_revive() const
	{
		return dead && lives >= 0 && dead_timer.getElapsedTime().asSeconds() > 2.f;
	}

	void revive()
	{
		dead = false;
	}

	bool is_grappling() const
	{
		return grappling != 0;
	}

	inline Grappable* target() const
	{
		return grapple_target;
	}

	void target(Grappable* new_target)
	{
		grapple_target = new_target;
		if (grapple_target)
			grappling = 1;
	}

	void step()
	{
		if (dead)
			return;

		// nothing to do if not grappling
		if (!grapple_target)
		{
			velocity += gravity * (float)game_step;
			position += velocity * (float)game_step;

			if (position.x > winw - half_height)
			{
				position.x = winw - half_height;
				velocity.x = velocity.x / -2.f;
			}
			else if (position.x < half_width)
			{
				position.x = half_width;
				velocity.x = velocity.x / -2.f;
			}

			// don't fall through floor
			if (position.y > winh - half_height)
			{
				velocity.x = 0.f;
				velocity.y = 0.f;
				position.y = winh - half_height;
			}
			return;
		}

		float d2 = dist2(position, grapple_target->pos());
		// need to move towards grapple
		if (d2 > max_grap_dist2)
		{
			// pull speed proportional to distance
			float speed = sqrtf(d2 - max_grap_dist2) * pull_speed_factor;
			// until you're close
			if (speed < min_pull_speed)
				speed = min_pull_speed;

			velocity = normv(grapple_target->pos() - position) * speed;

			position += velocity * (float)game_step;
		}
		// if we're close enough, start swinging
		else if (grappling == 1)
		{
			grap_dist = dist(position, grapple_target->pos());
			grappling = 2;
			last_target_pos = grapple_target->pos();
			swing_vel = (position.x < grapple_target->pos().x ? 1 : -1) * ((position.y - grapple_target->pos().y) + grap_dist) * starting_swing_vel / 2.f;
		}

		// if swinging
		if (grappling == 2)
		{
			// direction to player
			sf::Vector2f grap = position - grapple_target->pos();
			// tanget of swing direction
			sf::Vector2f grap_perp {grap.y, -grap.x};
			// normalized
			sf::Vector2f grap_perpn = normv(grap_perp);

			// naive velocity/position update
			swing_vel += dot(gravity, grap_perpn) * (float)game_step;
			velocity = grap_perpn * swing_vel;
			position += velocity * (float)game_step;

			// force position into grapple distance
			sf::Vector2f delta = position - grapple_target->pos();
			position = grapple_target->pos() + delta * (grap_dist / norm(delta));

			last_target_pos = grapple_target->pos();
		}
	}

	// aim and find nearest grapple to aim
	float aim(const sf::Vector2f& dir, const std::vector<Swinger*>& players, const std::list<Point*>& points, const sf::View& camera)
	{
		float theta = atan2f(dir.y, dir.x);
		aimbox.setRotation(rad2deg(theta));
		aiming = true;

		nearest = nullptr;
		float ndist2;

		for (auto& player : players)
		{
			// can't grapple self
			if (player == this)
				continue;

			// can't grapple someone grappling self
			if (player->target() == this)
				continue;

			// skip players already being grappled
			bool already_targeted = false;
			for (auto& player2 : players)
			{
				if (player2->target() == player)
				{
					already_targeted = true;
					break;
				}
			}
			if (already_targeted)
				continue;

			float ldist2 = dist2line(dir, player->pos());
			if (ldist2 < 0.f)
				continue;

			if (nearest == nullptr || ldist2 < ndist2)
			{
				nearest = player;
				ndist2 = ldist2;
			}
		}

		float top = camera.getCenter().y - camera.getSize().y / 2.f;

		for (auto& point : points)
		{
			// can't target stuff off screen
			if (point->pos().y < top)
				continue;
			// skip points already being grappled
			bool already_targeted = false;
			for (auto& player : players)
			{
				if (player->target() == point)
				{
					already_targeted = true;
					break;
				}
			}
			if (already_targeted)
				continue;

			float ldist2 = dist2line(dir, point->pos());
			if (ldist2 < 0.f)
				continue;

			if (nearest == nullptr || ldist2 < ndist2)
			{
				nearest = point;
				ndist2 = ldist2;
			}
		}

		return theta;
	}

	void stop_aim()
	{
		aiming = false;
		nearest = nullptr;
	}

	// return distance squared from p to the ray from position to position+dir (or -1 if not near ray)
	float dist2line(const sf::Vector2f& dir, const sf::Vector2f& p)
	{
		float dt = dot(p - position, dir) / (norm(p - position) * norm(dir));
		if (dt <= 0.7071f)
			return -1.f;
		if (dist2(p, position) > max_target_dist2)
			return -1.f;

		float num = dir.y * p.x - dir.x * p.y + position.y * (position.x + dir.x) - position.x * (position.y + dir.y);
		float ldist2 = (num * num) / norm2(dir);

		return ldist2;
	}

	void grapple()
	{
		if (nearest)
			target(nearest);
	}

	void let_go()
	{
		grappling = 0;
		if (grapple_target)
			velocity += grapple_target->vel();
		grapple_target = nullptr;
		return;
	}

	void draw_rope_on(sf::RenderTexture& render_target)
	{
		if (grapple_target == nullptr)
			return;

		auto bounds = rope.getLocalBounds();
		rope.setScale(4.f, 4.f);
		rope.setTextureRect(sf::IntRect {0, 0, (int)(dist(position, grapple_target->pos()) / 4.f), (int)bounds.height});

		rope.setPosition(position);
		sf::Vector2f dir = grapple_target->pos() - position;
		rope.setRotation(rad2deg(atan2f(dir.y, dir.x)));
		render_target.draw(rope);
	}

	bool is_speaking()
	{
		return texttimer.getElapsedTime().asSeconds() < texttime;
	}

	void draw_on(sf::RenderTexture& render_target, const sf::View& camera)
	{
		avatar.setPosition(position);
		render_target.draw(avatar);

		// textbox
		if (is_speaking())
		{
			auto& center = camera.getCenter();
			auto& size = camera.getSize();

			sf::Vector2f boxcorner {0.f, center.y - size.y / 2.f + 20.f + 2.f * half_height};
			if (index)
			{
				boxcorner.x = center.x + size.x / 2.f - 15.f - textbounds.width;
			}
			else
			{
				boxcorner.x = center.x - size.x / 2.f + 10.f;
			}
			sf::Vector2f boxcenter = boxcorner + sf::Vector2f{textbounds.width / 2.f, textbounds.height / 2.f};

			textboxbox.setPosition(boxcorner);
			render_target.draw(textboxbox);

			textarrow.setPosition(boxcenter);
			textarrow.setScale(dist(boxcenter, position) / 2.f, 1.f);
			textarrow.setRotation(rad2deg(atan2f(position.y - boxcenter.y, position.x - boxcenter.x)));
			render_target.draw(textarrow);

			textbox.setPosition(boxcorner);
			render_target.draw(textbox);
		}
	}

	void draw_target_on(sf::RenderTexture& render_target)
	{
		if (aiming)
		{
			aimbox.setPosition(position);
			render_target.draw(aimbox);

			if (nearest)
			{
				reticle.setPosition(nearest->pos());
				reticle.setRotation(game_time * 10 + 45 * index);
				render_target.draw(reticle);
			}
		}
	}

	void draw_lives_on(sf::RenderTexture& render_target)
	{
		for (int i = 0; i < lives; ++i)
		{
			avatar.setPosition(sf::Vector2f{index * winw - (30.f + i * 60.f) * (2 * index - 1), 30.f});
			render_target.draw(avatar);
		}
	}
};

int main(int argc, char* argv[])
{
	sf::VideoMode mode = sf::VideoMode::getFullscreenModes()[0];
	winw = mode.width;
	winh = mode.height;
	sf::RenderWindow window {mode, "TODO", sf::Style::Fullscreen};

	sf::RenderTexture render_target;
	if (!render_target.create(winw, winh))
	{
		std::cerr << "Failed to create render texture\n";
		return 1;
	}
	if (!sf::Shader::isAvailable())
	{
		std::cerr << "Shaders not available\n";
		return 1;
	}

	sf::Shader fx;
	if (!fx.loadFromFile("fragment.glsl", sf::Shader::Fragment))
	{
		std::cerr << "Failed to load fragment shader\n";
		return 1;
	}
	fx.setParameter("texture", sf::Shader::CurrentTexture);
	fx.setParameter("winw", (float)winw);
	fx.setParameter("winh", (float)winh);

	std::srand(time(nullptr));

	gravity.x = 0.f;
	gravity.y = 0.003f;

	sf::Font font;
	font.loadFromFile("/usr/share/fonts/TTF/DejaVuSansMono.ttf");

	// load textures
	sf::Texture bg_tex;
	if (!load(bg_tex, "img/bg.png"))
		return 1;
	bg_tex.setRepeated(true);

	sf::Texture floor_tex;
	if (!load(floor_tex, "img/floor.png"))
		return 1;

	sf::Texture inst_tex;
	if (!load(inst_tex, "img/inst.png"))
		return 1;

	sf::Texture snap_tex;
	if (!load(snap_tex, "img/snap.png"))
		return 1;

	sf::Texture avatar_tex;
	if (!load(avatar_tex, "img/player.png"))
		return 1;

	sf::Texture reticle_tex;
	if (!load(reticle_tex, "img/reticle.png"))
		return 1;

	sf::Texture aimbox_tex;
	if (!load(aimbox_tex, "img/aimbox.png"))
		return 1;

	sf::Texture rope_tex;
	if (!load(rope_tex, "img/rope.png"))
		return 1;
	rope_tex.setRepeated(true);

	sf::Texture point_tex;
	if (!load(point_tex, "img/point.png"))
		return 1;

	for (int i = 0; i < 2; ++i)
	{
		if (!sf::Joystick::isConnected(i))
		{
			std::cerr << "Need 2 joysticks\n";
			return 1;
		}
	}

	bool restart = true;
	while (restart)
	{
		sf::View camera = render_target.getDefaultView();
		float camera_speed_factor = -0.0005f;

		bool intro = true;
		bool gameover = false;

		sf::Sprite bg {bg_tex};
		float bg_scale = 4.f;
		auto bg_s = bg_tex.getSize();
		bg.setScale(bg_scale, bg_scale);
		bg.setTextureRect(sf::IntRect{0, 0, bg_s.x, winh / bg_scale + bg_s.y});
		bg.setPosition(0, -(int)bg_s.y);

		sf::Sprite floor {floor_tex};
		floor.setScale(4.f, 4.f);
		floor.setPosition(0, winh - 30.f);

		sf::Sprite inst {inst_tex};
		auto s = inst_tex.getSize();
		inst.setOrigin(s.x / 2.f, s.y / 2.f);
		inst.setScale(4.f, 4.f);
		inst.setPosition(winw / 2.f, winh / 2.f);

		sf::Sprite snap {snap_tex};
		s = snap_tex.getSize();
		snap.setOrigin(s.x / 2.f, s.y / 2.f);
		snap.setScale(4.f, 4.f);
		snap.setPosition(winw / 2.f, winh / 2.f - winh);

		std::vector<Swinger*> players;
		players.push_back(new Swinger {
			0,
			"GIUSEPPE",
			font,
			1.f * winw / 3.f,
			sf::Color {203, 40, 20},
			avatar_tex,
			reticle_tex,
			aimbox_tex,
			rope_tex
		});
		players.push_back(new Swinger {
			1,
			"FRANK",
			font,
			2.f * winw / 3.f,
			sf::Color {243, 166, 10},
			avatar_tex,
			reticle_tex,
			aimbox_tex,
			rope_tex
		});

		sf::Clock timer;
		game_time = 0.f;
		float game_start_time = 0.f;
		sf::Clock frame_timer;
		unsigned int last_frame_time = 0;

		sf::Text got;
		got.setFont(font);
		got.setCharacterSize(32);

		float min_dist = 150.f;
		float easy_dist = 350.f;
		float hard_dist = 400.f;

		std::list<Point*> points;
		// starting points
		points.push_back(new Point {1.f * winw / 3.f, winh - 400.f, point_tex});
		points.push_back(new Point {2.f * winw / 3.f, winh - 400.f, point_tex});
		// ladder
		points.push_back(new Point {2.f * winw / 3.f + 80.f, winh - 500.f, point_tex});
		points.push_back(new Point {2.f * winw / 3.f + 80.f, winh - 650.f, point_tex});
		// long grapple
		points.push_back(new Point {2.f * winw / 3.f - 450.f, winh - 800.f, point_tex});

		// segue to normal gen
		points.push_back(new Point {winw / 2.f - 300.f, winh - 1000.f, point_tex});
		points.push_back(new Point {winw / 2.f - 150.f, winh - 1000.f, point_tex});

		sf::RectangleShape bomb {sf::Vector2f{70.f, 30.f}};
		bomb.setFillColor(sf::Color{180, 180, 180});

		float highest_point = 0.f;
		for (auto& point : points)
		{
			if (point->pos().y < highest_point)
				highest_point = point->pos().y;
		}

		camera.zoom(0.5f);
		camera.setCenter(winw / 2.f, winh / 2.f + 300.f);

		bool running = true;
		bool cutscene = true;
		int cutphase = 0;
		while (cutscene && running)
		{
			// input
			sf::Event event;
			while (window.pollEvent(event))
			{
				if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Key::Escape))
				{
					running = false;
					restart = false;
				}
				if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Key::R)
				{
					running = false;
				}
			}
			if (cutphase == 0 && sf::Joystick::isButtonPressed(0, 7) && sf::Joystick::isButtonPressed(1, 7))
			{
				players[0]->say("FRANK! THE FLOOR IS LAVA!", 2);
				cutphase = 1;
			}

			if (cutphase == 1 && !players[0]->is_speaking())
			{
				players[1]->say("GIUSEPPE! WHAT DO WE DO NOW?", 2);
				cutphase = 2;
			}

			if (cutphase == 2 && !players[1]->is_speaking())
			{
				cutscene = false;
			}

			// draw on render texture
			render_target.setView(camera);
			render_target.clear();
			render_target.draw(bg);
			render_target.draw(floor);
			for (auto& point : points)
				point->draw_on(render_target);
			for (auto& player : players)
				player->draw_on(render_target, camera);

			// gui
			render_target.setView(render_target.getDefaultView());
			render_target.display();

			// draw with full screen effects
			fx.setParameter("time", game_time);

			window.clear();
			window.draw(sf::Sprite {render_target.getTexture()}, &fx);
			window.display();

			last_frame_time += frame_timer.getElapsedTime().asMilliseconds();
			frame_timer.restart();
			game_time = timer.getElapsedTime().asSeconds();
		}

		// transition to normal camera
		while (running)
		{
			float zdiff = render_target.getDefaultView().getSize().x / camera.getSize().x;
			if (zdiff < 1.01f)
				break;
			camera.zoom((zdiff - 1.f) / 2.f + 1.f);
			camera.move((render_target.getDefaultView().getCenter() - camera.getCenter()) / 3.f);

			// draw on render texture
			render_target.setView(camera);
			render_target.clear();
			render_target.draw(bg);
			render_target.draw(floor);
			for (auto& point : points)
				point->draw_on(render_target);
			for (auto& player : players)
				player->draw_on(render_target, camera);

			// gui
			render_target.setView(render_target.getDefaultView());
			render_target.display();

			// draw with full screen effects
			fx.setParameter("time", game_time);

			window.clear();
			window.draw(sf::Sprite {render_target.getTexture()}, &fx);
			window.display();

			last_frame_time += frame_timer.getElapsedTime().asMilliseconds();
			frame_timer.restart();
			game_time = timer.getElapsedTime().asSeconds();
		}
		camera = render_target.getDefaultView();

		while (running)
		{
			// input
			sf::Event event;
			while (window.pollEvent(event))
			{
				if (event.type == sf::Event::Closed || (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Key::Escape))
				{
					running = false;
					restart = false;
				}
				if (event.type == sf::Event::JoystickButtonPressed)
				{
					if (event.joystickButton.joystickId < players.size())
					{
						if (event.joystickButton.button == 0)
							players[event.joystickButton.joystickId]->grapple();
						else if (!intro && event.joystickButton.button == 1)
							players[event.joystickButton.joystickId]->let_go();
					}
				}
				if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Key::R)
				{
					running = false;
				}
			}

			if (!gameover)
			{
				for (unsigned int i = 0; i < players.size(); ++i)
				{
					sf::Vector2f aim {sf::Joystick::getAxisPosition(i, sf::Joystick::Axis::X), sf::Joystick::getAxisPosition(i, sf::Joystick::Axis::Y)};
					// deadzone check
					if (norm(aim) > 50.f)
						players[i]->aim(aim, players, points, camera);
					else
						players[i]->stop_aim();
				}
			}

			float bottom = camera.getCenter().y + camera.getSize().y / 2.f;

			// remove points that are off the bottom
			for (auto it = points.begin(); it != points.end();)
			{
				if ((*it)->pos().y > bottom)
				{
					for (auto& player : players)
					{
						if (player->target() == *it)
							player->let_go();
					}
					it = points.erase(it);
				}
				else
					++it;
			}

			float top = camera.getCenter().y - camera.getSize().y / 2.f;

			if (top < bg.getPosition().y)
			{
				bg.move(0, -(int)bg_s.y * 4.f);
			}

			if (!gameover)
			{
				// kill players
				for (auto& player : players)
				{
					if (player->is_dead())
						continue;

					if (player->pos().y > bottom + 120.f || (!intro && player->pos().y >= winh - player->get_half_height()))
					{
						player->die();

						for (auto& ps : players)
						{
							if (ps->target() == player)
								ps->let_go();
							else if (ps != player && ps->is_grappling())
							{
								ps->lament(player->get_name());
							}
						}

						if (player->get_lives() < 0)
						{
							if (!gameover)
							{
								gameover = true;
								std::stringstream s;
								s << "GAME OVER. SCORE: " << (render_target.getDefaultView().getCenter().y - camera.getCenter().y);
								got.setString(s.str());
								auto bounds = got.getLocalBounds();
								got.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
								got.setPosition(winw / 2.f, winh / 2.f);
							}
							continue;
						}

					}
				}

				// revive players
				for (auto& player : players)
				{
					if (!player->need_revive())
						continue;

					for (auto it = points.rbegin(); it != points.rend(); ++it)
					{
						if ((*it)->pos().y > top)
						{
							bool good = true;
							for (auto& pl : players)
							{
								if (pl->target() == *it)
								{
									good = false;
									break;
								}
							}
							if (good)
							{
								player->target(*it);
								player->revive();
								break;
							}
						}
					}
				}
			}

			// generate level if the highest point is on the screen
			if (!intro && highest_point > top)
			{
				float last_highest = highest_point;
				int last_size = points.size();
				// generate 1-4 more points
				unsigned int new_points = rand() % 3 + 2;

				while (points.size() - last_size < new_points)
				{
					for (auto& point : points)
					{
						// random angle
						int side = rand() % 2;
						float theta = -M_PI / 8.f - (rand() / (float)RAND_MAX) * M_PI / -8.f - (side * 5 * M_PI) / 8.f;

						int difficulty = rand() % 2 == 0 ? easy_dist : hard_dist;

						sf::Vector2f p {point->pos().x + cosf(theta) * difficulty, point->pos().y + sinf(theta) * difficulty};

						// want it in bounds and at least one point higher than the previous
						// XXX copied from Swinger class
						if (p.y < last_highest && p.x > 200.f && p.x < winw - 200.f)
						{
							// make sure it isn't too close to other points
							bool bad = false;
							for (auto& ps : points)
							{
								if (dist2(ps->pos(), p) < min_dist * min_dist)
								{
									bad = true;
									break;
								}
							}
							if (!bad)
							{
								points.push_back(new Point {p.x, p.y, point_tex});

								if (p.y < highest_point)
								{
									highest_point = p.y;
								}
								// need to break because iterator is invalid now
								break;
							}
						}
					}
				}
			}

			// game step
			while (game_step > 0 && last_frame_time > game_step)
			{
				for (auto& player : players)
					player->step();

				if (intro)
				{
					bool intro_done = true;
					for (auto& player : players)
					{
						if (!player->is_grappling())
						{
							intro_done = false;
							break;
						}
					}
					if (intro_done)
					{
						intro = false;
						game_start_time = game_time;
					}
				}
				if (!intro)
				{
					float camera_speed = (game_time - game_start_time) * camera_speed_factor;
					camera.move(0, camera_speed * game_step);
				}

				last_frame_time -= game_step;
			}

			// draw on render texture
			render_target.setView(camera);
			render_target.clear();
			render_target.draw(bg);
			render_target.draw(floor);

			render_target.draw(inst);
			render_target.draw(snap);

			for (auto& player : players)
				player->draw_rope_on(render_target);
			for (auto& point : points)
				point->draw_on(render_target);
			for (auto& player : players)
				player->draw_on(render_target, camera);
			for (auto& player : players)
				player->draw_target_on(render_target);

			// gui
			render_target.setView(render_target.getDefaultView());
			if (gameover)
				render_target.draw(got);

			for (auto& player : players)
				player->draw_lives_on(render_target);

			render_target.display();

			// draw with full screen effects
			fx.setParameter("time", game_time);

			window.clear();
			window.draw(sf::Sprite {render_target.getTexture()}, &fx);
			window.display();

			last_frame_time += frame_timer.getElapsedTime().asMilliseconds();
			frame_timer.restart();
			game_time = timer.getElapsedTime().asSeconds();
		}

		// cleanup
		for (auto& player : players)
			delete player;
		for (auto& point : points)
			delete point;
	}

	return 0;
}
