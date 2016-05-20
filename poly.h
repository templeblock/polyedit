#pragma once
#include "stdafx.h"
#include "point.h"

class Poly {
public:
	Poly();
	Poly(sf::Color _fillcolor);
	Poly(Point* _p1, Point* _p2, Point* _p3, int _s1, int _s2, int _s3);
	Poly(Point* _p1, Point* _p2, Point* _p3, int _s1, int _s2, int _s3, sf::Color _fillcolor);
	~Poly();
	Point* p1;
	Point* p2;
	Point* p3;
	sf::Vector2f center;
	int s1; // sx: Indices to rpoints for px
	int s2;
	int s3;
	int sa[3]; // Array autofilled from constructor s1,2,3 but edit this and call updatePointsToArray // Fix this
	sf::Color fillcolor;
	sf::ConvexShape cshape;
	void updateCShape();
	bool isWireframe = false;
	void updatePointers(std::vector<Point>& pts);
	void updatePointsToArray();
	void dbgPrintAllPoints();
	void updateCenter();
	bool selected = false;
};

