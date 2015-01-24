#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>

#include <SFML/Graphics.hpp>

const unsigned int winw = 1600;
const unsigned int winh = 900;
const unsigned int game_step = 16;
sf::Vector2f gravity;

using std::rand;

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
};

class Point : public Grappable
{
	sf::CircleShape circle;
public:
	Point(float x, float y)
		: Grappable {x, y}, circle {10.f}
	{
		circle.setOrigin(10.f, 10.f);
		circle.setPosition(position);
		circle.setFillColor(sf::Color {0, 0, 255});
	}

	void draw_on(sf::RenderTexture& render_target)
	{
		render_target.draw(circle);
	}
};

class Swinger : Grappable
{
	sf::RectangleShape rectangle;
	sf::RectangleShape reticle;
	sf::ConvexShape aimbox;

	Grappable* grapple_target = nullptr;
	Grappable* nearest = nullptr;
	// 0 = not grappling, 1 = moving toward point, 2 = swingin'
	int grappling = 0;
	int swing_dir = 1;

	float grap_dist = 200.f;
	float grap_dist2;

	float pull_speed_factor = 0.04f;
	float min_pull_speed = 1.f;
	float swing_speed_factor = 10.f;
	float min_base_speed = 0.2f;

	float aiming = false;

	float max_grapple_dist = 500.f;
	float max_grapple_dist2;

	int need_center = 0;

	sf::Vector2f last_target_pos;
public:
	Swinger(float x, float y)
		: Grappable {x, y}, rectangle {sf::Vector2f{40.f, 40.f}}, reticle {sf::Vector2f{20.f, 20.f}}
	{
		rectangle.setOrigin(20.f, 20.f);
		reticle.setOrigin(10.f, 10.f);
		reticle.setFillColor(sf::Color {0, 0, 0, 0});
		reticle.setOutlineColor(sf::Color {255, 0, 0});
		reticle.setOutlineThickness(2.f);

		aimbox.setPointCount(4);
		aimbox.setPoint(0, sf::Vector2f{40, 50});
		aimbox.setPoint(1, sf::Vector2f{300, 60});
		aimbox.setPoint(2, sf::Vector2f{300, -60});
		aimbox.setPoint(3, sf::Vector2f{40, -50});
		aimbox.setFillColor(sf::Color {255, 255, 0, 50});

		grap_dist2 = grap_dist * grap_dist;
		max_grapple_dist2 = max_grapple_dist * max_grapple_dist;
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
		// nothing to do if not grappling
		if (!grapple_target)
		{
			velocity += gravity * (float)game_step;
			position += velocity * (float)game_step;

			// don't fall through floor
			if (position.y > winh - 20.f)
			{
				velocity.y = 0.f;
				position.y = winh - 20.f;
			}
			return;
		}

		float d2 = dist2(position, grapple_target->pos());
		// need to move towards grapple
		if (d2 > grap_dist2)
		{
			// pull speed proportional to distance
			float speed = sqrtf(d2 - grap_dist2) * pull_speed_factor;
			// until you're close
			if (speed < min_pull_speed)
				speed = min_pull_speed;

			position += normv(grapple_target->pos() - position) * speed;
		}
		// if we're close enough, start swinging
		else if (grappling == 1)
		{
			grappling = 2;
			last_target_pos = grapple_target->pos();
		}

		// if swinging
		if (grappling == 2)
		{
			sf::Vector2f grap = position - grapple_target->pos();
			sf::Vector2f grap_perp {grap.y, -grap.x};
			sf::Vector2f grap_perpn = normv(grap_perp);
			sf::Vector2f grap_accel = grap_perpn * dot(gravity, grap_perpn);
			velocity += grap_accel * (float)game_step;
			position += velocity * (float)game_step;
			sf::Vector2f delta = position - grapple_target->pos();
			position = grapple_target->pos() + delta * (grap_dist / norm(delta));

			last_target_pos = grapple_target->pos();
		}
	}

	// aim and find nearest grapple to aim
	float aim(const sf::Vector2f& dir, const std::vector<Swinger*>& players, const std::vector<Point*>& points)
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

			float ldist2 = dist2line(dir, player->pos());
			if (ldist2 < 0.f)
				continue;

			if (nearest == nullptr || ldist2 < ndist2)
			{
				nearest = player;
				ndist2 = ldist2;
			}
		}

		for (auto& point : points)
		{
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
		if (dot(p - position, dir) <= 0.f)
			return -1.f;
		if (dist2(p, position) > max_grapple_dist2)
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

	void draw_on(sf::RenderTexture& render_target)
	{
		rectangle.setPosition(position);
		render_target.draw(rectangle);
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
				render_target.draw(reticle);
			}
		}
	}
};

int main(int argc, char* argv[])
{
	sf::RenderWindow window {sf::VideoMode {winw, winh}, "Test"};

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
	gravity.y = 0.005f;

	bool restart = true;
	while (restart)
	{
		sf::Color background {0, 0, 0};

		std::vector<Swinger*> players;
		players.push_back(new Swinger {winw / 2.f - 20.f, winh - 300.f});
		players.push_back(new Swinger {winw / 2.f + 20.f, winh - 300.f});

		sf::Clock timer;
		sf::Clock frame_timer;
		int last_frame_time = 0;

		std::vector<Point*> points;
		points.push_back(new Point {winw / 3.f, winh / 2.f - 100.f});
		points.push_back(new Point {2.f * winw / 3.f, winh / 2.f - 100.f});
		points.push_back(new Point {winw / 2.f, winh / 2.f - 50.f});

		bool running = true;
		while (running)
		{
			sf::Event event;
			while (window.pollEvent(event))
			{
				if (event.type == sf::Event::Closed)
				{
					running = false;
					restart = false;
				}
				if (event.type == sf::Event::JoystickButtonPressed && event.joystickButton.button == 0)
				{
					if (event.joystickButton.joystickId < players.size())
					{
						players[event.joystickButton.joystickId]->grapple();
					}
				}
				if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Key::R)
				{
					running = false;
				}
			}

			for (int i = 0; i < players.size(); ++i)
			{
				sf::Vector2f aim {sf::Joystick::getAxisPosition(i, sf::Joystick::Axis::X), sf::Joystick::getAxisPosition(i, sf::Joystick::Axis::Y)};

				// deadzone check
				if (norm(aim) > 50.f)
					players[i]->aim(aim, players, points);
				else
					players[i]->stop_aim();
			}

			while (game_step > 0 && last_frame_time > game_step)
			{
				for (auto& player : players)
					player->step();

				last_frame_time -= game_step;
			}

			// draw on render texture
			render_target.clear(background);
			for (auto& point : points)
				point->draw_on(render_target);
			for (auto& player : players)
				player->draw_on(render_target);
			for (auto& player : players)
				player->draw_target_on(render_target);
			render_target.display();

			// draw with full screen effects
			fx.setParameter("time", timer.getElapsedTime().asSeconds());

			window.clear();
			window.draw(sf::Sprite {render_target.getTexture()}, &fx);
			window.display();

			last_frame_time += frame_timer.getElapsedTime().asMilliseconds();
			frame_timer.restart();
		}

		// cleanup
		for (auto& player : players)
			delete player;
		for (auto& point : points)
			delete point;
	}

	return 0;
}
