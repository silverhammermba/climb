#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>

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

	sf::Clock timer;
	sf::Clock frame_timer;
	int game_step = 16;
	int last_frame_time = 0;

	sf::Vector2f grapple {winw / 2.f, winh / 2.f - 100.f};
	sf::CircleShape gcirc {20.f};
	gcirc.setPosition(grapple);
	gcirc.setFillColor(sf::Color {0, 0, 255});

	// 0 = not grappling, 1 = moving toward point, 2 = swingin'
	int grappling = 0;
	float swing_dir = 1.f;

	float grap_dist = 100.f;
	float grap_dist2 = grap_dist * grap_dist;

	bool running = true;
	while (running)
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				running = false;
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
				grappling = 1;

			float d2 = dist2(pos, grapple);
			// need to move towards grapple
			if (d2 > grap_dist2)
			{
				float speed = sqrtf(d2 - grap_dist2) * 0.02f;
				if (speed < 1.f)
					speed = 1.f;
				pos += normv(grapple - pos) * speed;
			}
			// if we're close enough, start swinging
			else if (grappling == 1)
			{
				grappling = 2;
			}

			// if swinging
			if (grappling == 2)
			{
				float theta = atan2f(pos.y - grapple.y, pos.x - grapple.x);

				float speed = sinf(theta);

				if (speed < 0.3f)
				{
					swing_dir *= -1.f;
					speed = 0.3f;
				}

				theta += swing_dir * speed / 10.f;

				pos.x = cosf(theta) * grap_dist + grapple.x;
				pos.y = sinf(theta) * grap_dist + grapple.y;
			}

			last_frame_time -= game_step;
		}

		rect.setPosition(pos);

		target.clear(background);
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
