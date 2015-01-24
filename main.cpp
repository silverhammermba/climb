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

float dist2(const sf::Vector2f& p1, const sf::Vector2f& p2)
{
	return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
}

float norm(const sf::Vector2f& v)
{
	return sqrtf(v.x * v.x + v.y * v.y);
}

sf::Vector2f normv(const sf::Vector2f& v)
{
	return v / norm(v);
}

class Grapple
{
	sf::Vector2f position;
	sf::CircleShape circle;
public:
	Grapple(float x, float y)
		: position {x, y}, circle {10.f}
	{
		circle.setOrigin(10.f, 10.f);
		circle.setPosition(position);
		circle.setFillColor(sf::Color {0, 0, 255});
	}

	inline const sf::Vector2f& pos() const
	{
		return position;
	}

	void draw_on(sf::RenderTexture& target)
	{
		target.draw(circle);
	}
};

class Swinger
{
	sf::Vector2f position;
	sf::RectangleShape rectangle;

	int grapple_target = -1;
	// 0 = not grappling, 1 = moving toward point, 2 = swingin'
	int grappling = 0;
	float swing_dir = 1.f;

	float grap_dist = 100.f;
	float grap_dist2;

	float pull_speed_factor = 0.04f;
	float min_pull_speed = 1.f;
	float swing_speed_factor = 0.1f;
	float min_swing_speed = 0.03f;
public:
	Swinger(float x, float y)
		: position {x, y}, rectangle {sf::Vector2f{40.f, 40.f}}
	{
		rectangle.setOrigin(20.f, 0.f);
		grap_dist2 = grap_dist * grap_dist;
	}

	bool is_grappling() const
	{
		return grappling != 0;
	}

	inline int target() const
	{
		return grapple_target;
	}

	void target(int new_target)
	{
		grapple_target = new_target;
		if (grapple_target >= 0)
			grappling = 1;
	}

	void grapple(const std::vector<Grapple>& grapples)
	{
		// nothing to do if not grappling
		if (grapple_target < 0)
			return;

		float d2 = dist2(position, grapples[grapple_target].pos());
		// need to move towards grapple
		if (d2 > grap_dist2)
		{
			// pull speed proportional to distance
			float speed = sqrtf(d2 - grap_dist2) * pull_speed_factor;
			// until you're close
			if (speed < min_pull_speed)
				speed = min_pull_speed;

			position += normv(grapples[grapple_target].pos() - position) * speed;
		}
		// if we're close enough, start swinging
		else if (grappling == 1)
		{
			grappling = 2;
		}

		// if swinging
		if (grappling == 2)
		{
			float theta = atan2f(position.y - grapples[grapple_target].pos().y, position.x - grapples[grapple_target].pos().x);

			float speed = sinf(theta) * swing_speed_factor;

			// you're swinging too high, swing back towards middle
			if (speed < min_swing_speed)
			{
				swing_dir = (position.x - grapples[grapple_target].pos().x) / std::abs(position.x - grapples[grapple_target].pos().x);
				speed = min_swing_speed;
			}

			theta += swing_dir * speed;

			position.x = cosf(theta) * grap_dist + grapples[grapple_target].pos().x;
			position.y = sinf(theta) * grap_dist + grapples[grapple_target].pos().y;
		}
	}

	void draw_on(sf::RenderTexture& target)
	{
		rectangle.setPosition(position);
		target.draw(rectangle);
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

	sf::Color background {0, 0, 0};

	Swinger p1 {winw / 2.f - 20.f, winh - 40.f};

	sf::Clock timer;
	sf::Clock frame_timer;
	int last_frame_time = 0;

	std::vector<Grapple> grapples;
	grapples.emplace(grapples.end(), winw / 3.f, winh / 2.f - 100.f);
	grapples.emplace(grapples.end(), 2.f * winw / 3.f, winh / 2.f - 100.f);

	bool running = true;
	while (running)
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				running = false;
			if (event.type == sf::Event::KeyReleased && event.key.code == sf::Keyboard::Key::Space)
			{
				if (p1.target() >= 0)
				{
					p1.target(1 - p1.target());
				}
			}
		}

		sf::Vector2f dir {0.f, 0.f};
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))
			dir.x -= 1.f;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right))
			dir.x += 1.f;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))
			dir.y -= 1.f;
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))
			dir.y += 1.f;

		while (game_step > 0 && last_frame_time > game_step)
		{
			// grapple
			if (!p1.is_grappling())
				p1.target(0);

			p1.grapple(grapples);

			last_frame_time -= game_step;
		}

		target.clear(background);
		for (auto& grapple : grapples)
			grapple.draw_on(target);
		p1.draw_on(target);
		target.display();

		fx.setParameter("time", timer.getElapsedTime().asSeconds());

		window.clear();
		window.draw(sf::Sprite {target.getTexture()}, &fx);
		window.display();

		last_frame_time += frame_timer.getElapsedTime().asMilliseconds();
		frame_timer.restart();
	}

	return 0;
}
