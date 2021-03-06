#include "stdafx.h"
#include "poly.h"

Poly::Poly()
{
}


Poly::~Poly()
{
}

Poly::Poly(sf::Color _fillcolor) {
	fillcolor = _fillcolor;
        // FIXME
	//p1, p2, p3 = &Point(0, 0, 5);
}
Poly::Poly(Point* _p1, Point* _p2, Point* _p3, int _s1, int _s2, int _s3) {
	p1 = _p1;
	p2 = _p2;
	p3 = _p3;
	fillcolor = sf::Color::White;
	s1 = _s1;
	s2 = _s2;
	s3 = _s3;
	sa[0] = s1;
	sa[1] = s2;
	sa[2] = s3;
	center.x = (p1->vector.x + p2->vector.x + p3->vector.x) / 3;
	center.y = (p1->vector.y + p2->vector.y + p3->vector.y) / 3;
}
Poly::Poly(Point* _p1, Point* _p2, Point* _p3, int _s1, int _s2, int _s3, sf::Color _fillcolor) {
	p1 = _p1;
	p2 = _p2;
	p3 = _p3;
	fillcolor = _fillcolor;
	s1 = _s1;
	s2 = _s2;
	s3 = _s3;
	sa[0] = s1;
	sa[1] = s2;
	sa[2] = s3;
	center.x = (p1->vector.x + p2->vector.x + p3->vector.x) / 3;
	center.y = (p1->vector.y + p2->vector.y + p3->vector.y) / 3;
}

void Poly::updateCShape(float viewzoom) {
	sf::ConvexShape ctshape;
	ctshape.setPointCount(3);
	ctshape.setPoint(0, p1->vector);
	ctshape.setPoint(1, p2->vector);
	ctshape.setPoint(2, p3->vector);
	if (!isWireframe){
		ctshape.setOutlineThickness(0);
		fillcolor.a = 255;
		ctshape.setFillColor(fillcolor);
		sf::Color outlinecolor = fillcolor;
		outlinecolor.a = 0;
		ctshape.setOutlineColor(outlinecolor);
	} 
	else {
		ctshape.setOutlineThickness(-1*viewzoom);
		fillcolor.a = 0;
		ctshape.setFillColor(fillcolor);
		sf::Color outlinecolor = fillcolor;
		outlinecolor.a = 255;
		ctshape.setOutlineColor(outlinecolor);
	}
	if (selected){
		ctshape.setOutlineThickness(-1*viewzoom);
		ctshape.setOutlineColor(sf::Color::Blue);
	}
	cshape = ctshape;
}

void Poly::dbgPrintAllPoints(){
	printf("p1: {%f, %f}; p2: {%f, %f}; p3: {%f, %f}\n", p1->vector.x, p1->vector.y, p2->vector.x, p2->vector.y, p3->vector.x, p3->vector.y);
}

void Poly::updatePointers(std::vector<Point>& pts){
	p1 = &pts[s1];
	p2 = &pts[s2];
	p3 = &pts[s3];
}

void Poly::updatePointsToArray(){
	s1 = sa[0];
	s2 = sa[1];
	s3 = sa[2];
}

void Poly::updateCenter(){
	center.x = (p1->vector.x + p2->vector.x + p3->vector.x) / 3;
	center.y = (p1->vector.y + p2->vector.y + p3->vector.y) / 3;
}
