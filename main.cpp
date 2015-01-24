#include <cmath>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <list>
#include <vector>

#include <SFML/Graphics.hpp>

const unsigned int winw = 1600;
const unsigned int winh = 900;
const unsigned int game_step = 16;
float game_time;
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

class Swinger : public Grappable
{
	sf::RectangleShape rectangle;
	sf::RectangleShape reticle;
	sf::ConvexShape aimbox;
	sf::RectangleShape rope;

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
public:
	Swinger(float x, float y, const sf::Color& color)
		: Grappable {x, y}, rectangle {sf::Vector2f{40.f, 40.f}}, reticle {sf::Vector2f{40.f, 40.f}}
	{
		rectangle.setOrigin(20.f, 20.f);
		rectangle.setFillColor(color);

		reticle.setOrigin(20.f, 20.f);
		reticle.setFillColor(sf::Color {0, 0, 0, 0});
		reticle.setOutlineColor(sf::Color {color.r * color.r, color.g * color.g, color.b * color.b});
		reticle.setOutlineThickness(3.f);

		aimbox.setPointCount(3);
		aimbox.setPoint(0, sf::Vector2f{40, 50});
		aimbox.setPoint(1, sf::Vector2f{80, 0});
		aimbox.setPoint(2, sf::Vector2f{40, -50});
		aimbox.setFillColor(sf::Color {color.r, color.g, color.b, 50});

		max_grap_dist2 = max_grap_dist * max_grap_dist;
		max_target_dist2 = max_target_dist * max_target_dist;

		rope.setOrigin(0.f, 1.5f);
		rope.setFillColor(sf::Color {color.r / 3, color.g / 3, color.b / 3});
	}

	int get_lives() const
	{
		return lives;
	}

	void die()
	{
		--lives;
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

			if (position.x > winw - 20.f)
			{
				position.x = winw - 20.f;
				velocity.x = velocity.x / -2.f;
			}
			else if (position.x < 20.f)
			{
				position.x = 20.f;
				velocity.x = velocity.x / -2.f;
			}

			// don't fall through floor
			if (position.y > winh - 20.f)
			{
				velocity.x = 0.f;
				velocity.y = 0.f;
				position.y = winh - 20.f;
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
		if (dot(p - position, dir) <= 0.f)
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
		rope.setSize(sf::Vector2f{dist(position, grapple_target->pos()), 3.f});
		rope.setPosition(position);
		sf::Vector2f dir = grapple_target->pos() - position;
		rope.setRotation(rad2deg(atan2f(dir.y, dir.x)));
		render_target.draw(rope);
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
				reticle.setRotation(game_time * 10);
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
	gravity.y = 0.003f;

	bool restart = true;
	while (restart)
	{
		sf::View camera = render_target.getDefaultView();
		float camera_speed_factor = -0.001f;

		bool intro = true;

		sf::Color background {0, 0, 0};

		std::vector<Swinger*> players;
		players.push_back(new Swinger {1.f * winw / 3.f - 20.f, winh - 300.f, sf::Color {203, 40, 20}});
		players.push_back(new Swinger {2.f * winw / 3.f + 20.f, winh - 300.f, sf::Color {243, 166, 10}});

		sf::Clock timer;
		game_time = 0.f;
		float game_start_time = 0.f;
		sf::Clock frame_timer;
		int last_frame_time = 0;

		float min_dist = 150.f;
		float easy_dist = 350.f;

		std::list<Point*> points;
		// starting points
		points.push_back(new Point {1.f * winw / 3.f, winh - 400.f});
		points.push_back(new Point {2.f * winw / 3.f, winh - 400.f});
		// ladder
		points.push_back(new Point {2.f * winw / 3.f + 80.f, winh - 500.f});
		points.push_back(new Point {2.f * winw / 3.f + 80.f, winh - 650.f});
		// long grapple
		points.push_back(new Point {2.f * winw / 3.f - 450.f, winh - 800.f});

		// segue to normal gen
		points.push_back(new Point {winw / 2.f - 300.f, winh - 1000.f});
		points.push_back(new Point {winw / 2.f - 150.f, winh - 1000.f});

		float highest_point = 0.f;
		for (auto& point : points)
		{
			if (point->pos().y < highest_point)
				highest_point = point->pos().y;
		}

		bool running = true;
		while (running)
		{
			// input
			sf::Event event;
			while (window.pollEvent(event))
			{
				if (event.type == sf::Event::Closed)
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

			for (int i = 0; i < players.size(); ++i)
			{
				sf::Vector2f aim {sf::Joystick::getAxisPosition(i, sf::Joystick::Axis::X), sf::Joystick::getAxisPosition(i, sf::Joystick::Axis::Y)};

				// deadzone check
				if (norm(aim) > 50.f)
					players[i]->aim(aim, players, points, camera);
				else
					players[i]->stop_aim();
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

			bool gameover = false;
			for (auto& player : players)
			{
				if (player->pos().y > bottom + 500.f && !player->is_grappling())
				{
					player->die();
					if (player->get_lives() < 0)
					{
						gameover = true;
						continue;
					}

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
								break;
							}
						}
					}
				}
			}

			if (gameover)
			{
				// TODO
			}

			// generate level if the highest point is on the screen
			if (!intro && highest_point > top)
			{
				float last_highest = highest_point;
				int last_size = points.size();
				bool new_high = false;
				// generate 1-4 more points
				int new_points = rand() % 4 + 1;

				while (points.size() - last_size < new_points)
				{
					for (auto& point : points)
					{
						// random angle
						float theta = (4.f * (rand() / (float)RAND_MAX) + 1.f) * M_PI / -6.f;
						sf::Vector2f p {point->pos().x + cosf(theta) * easy_dist, point->pos().y + sinf(theta) * easy_dist};

						// want it in bounds and at least one point higher than the previous
						if (p.y < last_highest && p.x > 0.f && p.x < winw)
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
								points.push_back(new Point {p.x, p.y});

								if (p.y < highest_point)
								{
									new_high = true;
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
			render_target.clear(background);
			render_target.setView(camera);
			for (auto& player : players)
				player->draw_rope_on(render_target);
			for (auto& point : points)
				point->draw_on(render_target);
			for (auto& player : players)
				player->draw_on(render_target);
			for (auto& player : players)
				player->draw_target_on(render_target);
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
