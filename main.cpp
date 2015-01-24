#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>

#include <SFML/Graphics.hpp>

const unsigned int winw = 1600;
const unsigned int winh = 900;
const unsigned int game_step = 16;

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
public:
	Grappable(float x, float y)
		: position {x, y}
	{}

	Grappable(const sf::Vector2f& v)
		: position {v}
	{}

	inline const sf::Vector2f& pos() const
	{
		return position;
	}
};

class Grapple : public Grappable
{
	sf::CircleShape circle;
public:
	Grapple(float x, float y)
		: Grappable {x, y}, circle {10.f}
	{
		circle.setOrigin(10.f, 10.f);
		circle.setPosition(position);
		circle.setFillColor(sf::Color {0, 0, 255});
	}

	void draw_on(sf::RenderTexture& target)
	{
		target.draw(circle);
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
	float swing_dir = 1.f;

	float grap_dist = 200.f;
	float grap_dist2;

	float pull_speed_factor = 0.04f;
	float min_pull_speed = 1.f;
	float swing_speed_factor = 10.f;
	float min_swing_speed = 0.008f;

	float aiming = false;

	float max_grapple_dist = 500.f;
	float max_grapple_dist2;

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

	void grapple()
	{
		// nothing to do if not grappling
		if (!grapple_target)
			return;

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
		}

		// if swinging
		if (grappling == 2)
		{
			float theta = atan2f(position.y - grapple_target->pos().y, position.x - grapple_target->pos().x);

			float speed = sinf(theta) * swing_speed_factor / grap_dist;

			// you're swinging too high, swing back towards middle
			if (speed < min_swing_speed)
			{
				swing_dir = (position.x - grapple_target->pos().x) / std::abs(position.x - grapple_target->pos().x);
				speed = min_swing_speed;
			}

			theta += swing_dir * speed;

			position.x = cosf(theta) * grap_dist + grapple_target->pos().x;
			position.y = sinf(theta) * grap_dist + grapple_target->pos().y;
		}
	}

	// aim and find nearest grapple to aim
	float aim(const sf::Vector2f& dir, std::vector<Grapple>& grapples)
	{
		float theta = atan2f(dir.y, dir.x);
		aimbox.setRotation(rad2deg(theta));
		aiming = true;

		nearest = nullptr;
		float ndist2;

		for (auto& grapple : grapples)
		{
			if (dot(grapple.pos(), position) <= 0.f)
				continue;
			if (dist2(grapple.pos(), position) > max_grapple_dist2)
				continue;

			float num = dir.y * grapple.pos().x - dir.x * grapple.pos().y + position.y * (position.x + dir.x) - position.x * (position.y + dir.y);
			float ldist2 = (num * num) / norm2(dir);

			if (nearest == nullptr || ldist2 < ndist2)
			{
				nearest = &grapple;
				ndist2 = ldist2;
			}
		}

		return theta;
	}

	void gogo()
	{
		if (nearest)
			target(nearest);
	}

	void draw_on(sf::RenderTexture& target)
	{
		rectangle.setPosition(position);
		target.draw(rectangle);

		if (aiming)
		{
			aimbox.setPosition(position);
			target.draw(aimbox);
			aiming = false;

			if (nearest)
			{
				reticle.setPosition(nearest->pos());
				target.draw(reticle);
			}
		}
	}
};

int main(int argc, char* argv[])
{
	sf::RenderWindow window {sf::VideoMode {winw, winh}, "Test"};

	sf::RenderTexture target;
	if (!target.create(winw, winh))
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

	bool restart = true;
	while (restart)
	{
		sf::Color background {0, 0, 0};

		std::vector<Swinger*> players;
		players.push_back(new Swinger {winw / 2.f - 20.f, winh - 20.f});
		players.push_back(new Swinger {winw / 2.f + 20.f, winh - 20.f});

		sf::Clock timer;
		sf::Clock frame_timer;
		int last_frame_time = 0;

		std::vector<Grapple> grapples;
		grapples.emplace(grapples.end(), winw / 3.f, winh / 2.f - 100.f);
		grapples.emplace(grapples.end(), 2.f * winw / 3.f, winh / 2.f - 100.f);
		grapples.emplace(grapples.end(), winw / 2.f, winh / 2.f - 50.f);

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
						players[event.joystickButton.joystickId]->gogo();
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
				{
					float theta = players[i]->aim(aim, grapples);
				}
			}

			while (game_step > 0 && last_frame_time > game_step)
			{
				for (auto& player : players)
					player->grapple();

				last_frame_time -= game_step;
			}

			// draw on render texture
			target.clear(background);
			for (auto& grapple : grapples)
				grapple.draw_on(target);
			for (auto& player : players)
				player->draw_on(target);
			target.display();

			// draw with full screen effects
			fx.setParameter("time", timer.getElapsedTime().asSeconds());

			window.clear();
			window.draw(sf::Sprite {target.getTexture()}, &fx);
			window.display();

			last_frame_time += frame_timer.getElapsedTime().asMilliseconds();
			frame_timer.restart();
		}
	}

	return 0;
}
