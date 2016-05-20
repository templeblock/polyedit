#include "stdafx.h"
#include "point.h"

Point::Point(){
	size = 5;
	vector = sf::Vector2f(0, 0);
	color = sf::Color::Green;
}
Point::Point(float s) {
	size = s;
	vector = sf::Vector2f(0, 0);
	color = sf::Color::Green;
}
Point::Point(float x, float y, float s) {
	size = s;
	vector = sf::Vector2f(x, y);
	color = sf::Color::Green;
}
Point::Point(sf::Vector2f v, float s) {
	size = s;
	vector = v;
	color = sf::Color::Green;
}
Point::Point(sf::Vector2f v, sf::Color c, float s) {
	size = s;
	vector = v;
	color = c;
}

Point::~Point()
{
}

void Point::updateCShape(float zoom){
	cshape.setRadius(size*zoom);
	sf::Vector2f pos = vector;
	pos.x -= size*zoom;
	pos.y -= size*zoom;
	cshape.setPosition(pos);
	cshape.setFillColor(sf::Color::Transparent);
	selected ? cshape.setOutlineColor(sf::Color::Blue) : cshape.setOutlineColor(sf::Color::Green);
	cshape.setOutlineThickness(-1.5*zoom);
}

bool Point::operator==(const Point& rhs){
	if (vector == rhs.vector){
		return true;
	}
	else {
		return false;
	}
}
