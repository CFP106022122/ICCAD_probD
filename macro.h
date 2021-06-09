class Macro {
private:
	double width, height;
	double _x1, _y1; // lower left coordinate
	double _x2, _y2; // upper right coordinate
	int _id;
	bool _is_fixed;
public:
	Macro(double w, double h, double _x, double _y, bool is_f, int i): width{w}, height{h}, _x1{_x}, _y1{_y}, _is_fixed{is_f}, _id{i} { 
		_x2 = _x1 + width;
		_y2 = _y1 + height;
	}
	
	double x1() const { return _x1; }
	double x2() const { return _x2; }
	double y1() const { return _y1; }
	double y2() const { return _y2; }
			
	int id() const { return _id; }

	double w() const { return width; }

	double h() const { return height; }

	bool is_fixed() const { return _is_fixed; }

	double cx() const { return _x1+width/2; }

	double cy() const { return _y1+height/2; }

	friend bool is_overlapped(Macro& m1, Macro& m2); 
	friend bool x_dir_is_overlapped_less(Macro& m1, Macro& m2);
	friend bool x_dir_projection_no_overlapped(Macro& m1, Macro& m2);
	friend bool y_dir_projection_no_overlapped(Macro& m1, Macro& m2);
	friend bool projection_no_overlapped(Macro& m1, Macro& m2);	
};
