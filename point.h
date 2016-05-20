#pragma once
class Point
{
public:
	Point();
	Point(float s);
	Point(float x, float y, float s);
	Point(sf::Vector2f v, float s);
	Point(sf::Vector2f v, sf::Color c, float s);
	~Point();
	sf::Vector2f vector;
	float size;
	sf::CircleShape cshape = sf::CircleShape(0);
	sf::Color color;
	bool selected = false;
	void updateCShape(float zoom);
	void adjSizeToZoom(float zoom);
	bool operator==(const Point& rhs);
};

