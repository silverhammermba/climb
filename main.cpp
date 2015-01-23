#include <cstdlib>
#include <ctime>
#include <iostream>

#include <SFML/Graphics.hpp>

using std::rand;

int main(int argc, char* argv[])
{
	sf::RenderWindow window {sf::VideoMode {800, 600}, "Test"};

	sf::RenderTexture target;
	if (!target.create(800, 600))
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

	std::srand(time(nullptr));

	sf::Color background {0, 0, 0};
	sf::Color new_color {0, 0, 0};

	sf::Vector2f pos {0.f, 0.f};
	sf::RectangleShape rect {sf::Vector2f {40.f, 40.f}};

	sf::Clock timer;

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

		pos += dir * 4.f;
		rect.setPosition(pos);

		// pick new background color if we're there
		if (new_color == background)
		{
			new_color.r = rand() % 256;
			new_color.g = rand() % 256;
			new_color.b = rand() % 256;
		}

		// move towards new background color
		if (new_color.r < background.r)
			background.r -= rand() % 2;
		else if (new_color.r > background.r)
			background.r += rand() % 2;
		else if (new_color.g < background.g)
			background.g -= rand() % 2;
		else if (new_color.g > background.g)
			background.g += rand() % 2;
		else if (new_color.b < background.b)
			background.b -= rand() % 2;
		else if (new_color.b > background.b)
			background.b += rand() % 2;

		target.clear(background);
		target.draw(rect);
		target.display();

		fx.setParameter("time", timer.getElapsedTime().asSeconds());

		window.clear();
		window.draw(sf::Sprite {target.getTexture()}, &fx);
		window.display();
	}

	return 0;
}
