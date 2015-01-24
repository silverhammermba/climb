#include <cstdlib>
#include <ctime>
#include <iostream>

#include <SFML/Graphics.hpp>

const unsigned int winw = 1600;
const unsigned int winh = 900;

using std::rand;

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

	sf::Vector2f pos {0.f, 0.f};
	sf::RectangleShape rect {sf::Vector2f {40.f, 40.f}};

	sf::Clock timer;
	sf::Clock frame_timer;
	int game_step = 16;
	int last_frame_time = 0;

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

		while (last_frame_time > game_step)
		{
			// step game world
			pos += dir * 4.f;

			last_frame_time -= game_step;
		}

		rect.setPosition(pos);

		target.clear(background);
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
