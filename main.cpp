#include <stdlib.h>
#include <time.h>

#include <SFML/Graphics.hpp>

int main(int argc, char* argv[])
{
	sf::RenderWindow window {sf::VideoMode {800, 600}, "Test"};

	srand(time(nullptr));

	sf::Color background {0, 0, 0};
	sf::Color new_color {0, 0, 0};

	bool running = true;
	while (running)
	{
		sf::Event event;
		while (window.pollEvent(event))
		{
			if (event.type == sf::Event::Closed)
				running = false;
		}

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

		window.clear(background);
		window.display();
	}

	return 0;
}
