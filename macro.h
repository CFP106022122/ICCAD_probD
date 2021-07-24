#ifndef _MACRO_H_
#define _MACRO_H_

#include <iostream>
#include <string>

using namespace std;

#define border 3
#define _fixed 2
#define placed 1

class Macro
{

private:
	double width, height;
	double _x1, _y1; // lower left coordinate
	double _x2, _y2; // upper right coordinate
	int _id;
	bool _is_fixed;

	//shao ting
	string macro_shape;
	int macro_type;
	string macro_name;

public:
	Macro(double w, double h, double _x, double _y, bool is_f, int i) : width{w}, height{h}, _x1{_x}, _y1{_y}, _is_fixed{is_f}, _id{i}
	{
		_x2 = _x1 + width;
		_y2 = _y1 + height;
		macro_shape = "null";
		macro_type = 3;
		macro_name = "null";
	}

	double x1() const { return _x1; }
	double x2() const { return _x2; }
	double y1() const { return _y1; }
	double y2() const { return _y2; }
	void updateXY(pair<double, double> p){
		this->_x1 = p.first - width/2;
		this->_x2 = p.first + width/2;
		this->_y1 = p.second - height/2;
		this->_y2 = p.second + height/2;
	}
	int id() const { return _id; }

	double w() const { return width; }

	double h() const { return height; }

	bool is_fixed() const { return _is_fixed; }

	double cx() const { return _x1 + width / 2; }

	double cy() const { return _y1 + height / 2; }

	friend bool is_overlapped(Macro &m1, Macro &m2);
	friend bool x_dir_is_overlapped_less(Macro &m1, Macro &m2);
	friend bool x_dir_projection_no_overlapped(Macro &m1, Macro &m2);
	friend bool y_dir_projection_no_overlapped(Macro &m1, Macro &m2);
	friend bool projection_no_overlapped(Macro &m1, Macro &m2);

	//shao ting
	Macro(string name, string shape, int type, double _x, double _y, bool is_f, int i) : macro_name{name}, macro_shape{shape}, macro_type{type}, width{-1}, height{-1}, _x1{_x}, _y1{_y}, _is_fixed{is_f}, _id{i}
	{
		_x2 = -1;
		_y2 = -1;
	}

	Macro(string name, double w, double h) : macro_name{name}, width{w}, height{h}
	{
		_x2 = -1;
		_y2 = -1;
		macro_shape = "null";
		macro_type = -1;
		_x1 = -1;
		_y1 = -1;
		_is_fixed = true;
		_id = -1;
	}

	int type() const { return macro_type; }

	string shape() const { return macro_shape; }

	string name() const { return macro_name; }

	void setWidthHeight(Macro m, int dbu_per_micron)
	{
		width = m.w() * (double)dbu_per_micron;
		height = m.h() * (double)dbu_per_micron;
		_x2 = _x1 + width;
		_y2 = _y1 + height;
	}

	Macro(Macro& m_copy){
		width = m_copy.width;
		height = m_copy.height;
		_x1 = m_copy._x1;
		_y1 = m_copy._y1;
		_x2 = m_copy._x2;
		_y2 = m_copy._y2;
		_id = m_copy._id;
		_is_fixed = m_copy._is_fixed;

		macro_shape = m_copy.macro_shape;
		macro_type = m_copy.macro_type;
		macro_name = m_copy.macro_name;
	}
};

#endif
