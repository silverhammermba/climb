#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>

#include <SFML/Graphics.hpp>

const unsigned int winw = 1600;
const unsigned int winh = 900;

using std::rand;

float dist2(sf::Vector2f& p1, sf::Vector2f& p2)
{
	return (p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y);
}

float norm(sf::Vector2f v)
{
	return sqrtf(v.x * v.x + v.y * v.y);
}

sf::Vector2f normv(sf::Vector2f v)
{
	return v / norm(v);
}

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

	sf::Vector2f pos {winw / 2.f - 20.f, winh - 40.f};
	sf::RectangleShape rect {sf::Vector2f {40.f, 40.f}};
	rect.setOrigin(20.f, 0.f);

	sf::Clock timer;
	sf::Clock frame_timer;
	int game_step = 16;
	int last_frame_time = 0;

	std::vector<sf::Vector2f> grapples;
	grapples.push_back(sf::Vector2f {winw / 3.f, winh / 2.f - 100.f});
	grapples.push_back(sf::Vector2f {2.f * winw / 3.f, winh / 2.f - 100.f});
	std::vector<sf::CircleShape> gcircs;
	for (int i = 0; i < grapples.size(); ++i)
	{
		gcircs.emplace(gcircs.end(), 10.f);
		gcircs[i].setOrigin(10.f, 10.f);
		gcircs[i].setPosition(grapples[i]);
		gcircs[i].setFillColor(sf::Color {0, 0, 255});
	}

	// 0 = not grappling, 1 = moving toward point, 2 = swingin'
	int grapple_target = -1;
	int grappling = 0;
	float swing_dir = 1.f;

	float grap_dist = 100.f;
	float grap_dist2 = grap_dist * grap_dist;

	float pull_speed_factor = 0.04f;
	float min_pull_speed = 1.f;
	float swing_speed_factor = 0.1f;
	float min_swing_speed = 0.03f;

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
				if (grapple_target >= 0)
				{
					grapple_target = 1 - grapple_target;
					grappling = 1;
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
			if (!grappling)
			{
				grapple_target = 0;
				grappling = 1;
			}

			float d2 = dist2(pos, grapples[grapple_target]);
			// need to move towards grapple
			if (d2 > grap_dist2)
			{
				// pull speed proportional to distance
				float speed = sqrtf(d2 - grap_dist2) * pull_speed_factor;
				// until you're close
				if (speed < min_pull_speed)
					speed = min_pull_speed;

				pos += normv(grapples[grapple_target] - pos) * speed;
			}
			// if we're close enough, start swinging
			else if (grappling == 1)
			{
				grappling = 2;
			}

			// if swinging
			if (grappling == 2)
			{
				float theta = atan2f(pos.y - grapples[grapple_target].y, pos.x - grapples[grapple_target].x);

				float speed = sinf(theta) * swing_speed_factor;

				// you're swinging too high, swing back towards middle
				if (speed < min_swing_speed)
				{
					swing_dir = (pos.x - grapples[grapple_target].x) / std::abs(pos.x - grapples[grapple_target].x);
					speed = min_swing_speed;
				}

				theta += swing_dir * speed;

				pos.x = cosf(theta) * grap_dist + grapples[grapple_target].x;
				pos.y = sinf(theta) * grap_dist + grapples[grapple_target].y;
			}

			last_frame_time -= game_step;
		}

		rect.setPosition(pos);

		target.clear(background);
		for (auto& gcirc : gcircs)
			target.draw(gcirc);
		target.draw(rect);
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
