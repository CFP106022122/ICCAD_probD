#include "macro.h"
#include <iostream>
using namespace std;

bool is_overlapped(Macro& m1, Macro& m2) {
	return !(m1._x2 < m2._x1 || m1._x1 > m2._x2 ||
             m1._y2 < m2._y1 || m1._y1 > m2._y2);
}

bool x_dir_is_overlapped_less(Macro& m1, Macro& m2) {
	double diff_x, diff_y;
	if (m1.cx() > m2.cx()) // make sure m1 is on the left.
		std::swap(m1, m2);
	diff_x = m1._x2 - m2._x1;
	if (m1.cy() < m2.cy())
		diff_y = m1._y2 - m2._y1;
	else
		diff_y = m2._y2 - m1._y1;
	return diff_x < diff_y; 
}

bool x_dir_projection_no_overlapped(Macro& m1, Macro& m2) {
	if (m1.cx() > m2.cx()) // make sure m1 is on the left.
		std::swap(m1, m2);
	return m2._x1 > m1._x2;
}

bool y_dir_projection_no_overlapped(Macro& m1, Macro& m2) {
	if (m1.cy() > m2.cy()) // make sure m1 is at the bottom.
		std::swap(m1, m2);
	return m2._y1 > m1._y2;
}

bool projection_no_overlapped(Macro& m1, Macro& m2) {
	return x_dir_projection_no_overlapped(m1, m2) && y_dir_projection_no_overlapped(m1, m2);  
}
