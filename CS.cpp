#include "corner_stitch/tiles/tile.h"
#include "corner_stitch/utils/update.h"
#include "macro.h"
#include <vector>

pair<double, double> transform_coor_clockwise(pair<double, double> c)
{
    return {c.second, -c.first};
}

pair<double, double> transform_coor_counter_clockwise(pair<double, double> c)
{
    return {-c.second, c.first};
}

void set_coor(int *coor, Macro *m, bool is_horizontal)
{
    // coor is clockwise
    if (is_horizontal)
    {
        coor[0] = (int)m->x1();
        coor[1] = (int)m->y1();
        coor[2] = (int)m->x1();
        coor[3] = (int)(m->y1() + m->h());
        coor[4] = (int)m->x2();
        coor[5] = (int)m->y2();
        coor[6] = (int)m->x2();
        coor[7] = (int)(m->y2() - m->h());
    }
    else
    {
        pair<double, double> new_rb = transform_coor_counter_clockwise({m->x1(), m->y1()});
        pair<double, double> new_lt = transform_coor_counter_clockwise({m->x2(), m->y2()});
        pair<double, double> new_lb = transform_coor_counter_clockwise({m->x1(), m->y1() + m->h()});
        pair<double, double> new_rt = transform_coor_counter_clockwise({m->x2(), m->y2() - m->h()});
        coor[0] = (int)new_lb.first;
        coor[1] = (int)new_lb.second;
        coor[2] = (int)new_lt.first;
        coor[3] = (int)new_lt.second;
        coor[4] = (int)new_rt.first;
        coor[5] = (int)new_rt.second;
        coor[6] = (int)new_rb.first;
        coor[7] = (int)new_rb.second;
    }
}

void createInitCS(vector<Macro *> after_lp_macros, Plane *plane, bool is_horizontal)
{
    for (auto &m : after_lp_macros)
    {
        int coor[8];
        set_coor(coor, m, is_horizontal);
        if ((coor[0] != coor[2]) || (coor[3] != coor[5]) || (coor[4] != coor[6]) || (coor[7] != coor[1]))
        {
            fprintf(stderr, "Skip invalid rectangle: (%d, %d) - (%d, %d) - (%d, %d) - (%d, %d)\n", coor[0], coor[1], coor[2], coor[3], coor[4], coor[5], coor[6], coor[7]);
            continue;
        }

        // insert tile into tile plane
        Rect rect = {{coor[0], coor[1]}, {coor[4], coor[5]}};
        if (InsertTile(&rect, plane) == NULL)
        {
            fprintf(stderr, "Invalid insertion due to the overlapping with existing rectangles: (%d, %d) - (%d, %d).\n", coor[0], coor[1], coor[4], coor[5]);
        }
    }
}
extern double chip_width;
extern double chip_height;

struct info
{
    info(double &_buff_cost, Plane *_now, Plane *_next, bool &_changed, bool _is_horizontal)
        : buff_cost{_buff_cost}, now{_now}, next{_next}, changed{_changed}, is_horizontal{_is_horizontal} {}
    double &buff_cost;
    Plane *now, *next;
    bool &changed;
    bool is_horizontal;
};

int marker(Tile *tile, ClientData cdata)
{
    info *i = (info *)cdata;
    bool is_horizontal = i->is_horizontal;
    int chip_w = (is_horizontal) ? (int)chip_width : int(chip_height);
    if (!TiGetBody(tile))
    {
        Rect rect, rect_the_other_dir;
        TiToRect(tile, &rect);
        int width = rect.r_ur.p_x - rect.r_ll.p_x;
        int height = rect.r_ur.p_y - rect.r_ll.p_y;
        pair<double, double> new_lb, new_rt;
        if (is_horizontal)
        {
            new_lb = transform_coor_counter_clockwise({rect.r_ll.p_x, rect.r_ll.p_y + height});
            new_rt = transform_coor_counter_clockwise({rect.r_ur.p_x, rect.r_ur.p_y - height});
        }
        else
        {
            new_lb = transform_coor_clockwise({rect.r_ur.p_x, rect.r_ur.p_y - height});
            new_rt = transform_coor_clockwise({rect.r_ll.p_x, rect.r_ll.p_y + height});
        }
        rect_the_other_dir.r_ll.p_x = new_lb.first;
        rect_the_other_dir.r_ll.p_y = new_lb.second;
        rect_the_other_dir.r_ur.p_x = new_rt.first;
        rect_the_other_dir.r_ur.p_y = new_rt.second;
        if (width < chip_w)
        {
            InsertTile(&rect, i->now);
            InsertTile(&rect_the_other_dir, i->next);
            i->changed = true;
            i->buff_cost += width * height;
        }
    }
    return 0;
}

void markSpaceTiles(Plane *plane_horizontal, Plane *plane_vertical, double &buff_cost)
{
    Plane *now = plane_horizontal, *next = plane_vertical, *tmp;
    bool changed;
    do
    {
        changed = false;
        pair<double, double> vertical_origin = transform_coor_counter_clockwise({0, chip_height});
        pair<double, double> vertical_rt = transform_coor_counter_clockwise({chip_width, 0});
        Rect rect;
        if (now == plane_horizontal)
        {
            rect = {{0, 0}, {(int)chip_width, (int)chip_height}};
        }
        else
        {
            rect = {{(int)vertical_origin.first, (int)vertical_origin.second}, {(int)vertical_rt.first, (int)vertical_rt.second}};
        }
        info i(buff_cost, now, next, changed, (now == plane_horizontal));
        TiSrArea(NULL, now, rect, marker, (ClientData)(&i));
        tmp = now;
        now = next;
        next = tmp;
    } while (changed);
}

struct Checker_package
{
    Checker_package(bool _ok, double d, Plane *p) : ok{_ok}, buffer_constraint{d}, plane{p} {}
    bool ok;
    double buffer_constraint;
    Plane *plane;
};

int checker_helper(Tile *tile, ClientData cdata)
{
    bool *legal = (bool *)cdata;
    if (TiGetBody(tile))
    {
        *legal = false;
        return 1;
    }
    return 0;
}

int checker(Tile *tile, ClientData cdata)
{
    Checker_package *p = (Checker_package *)cdata;
    if (TiGetBody(tile))
    {
        Rect rect, buffer_area;
        TiToRect(tile, &rect);
        buffer_area.r_ll.p_x = rect.r_ll.p_x - (p->buffer_constraint);
        buffer_area.r_ll.p_y = rect.r_ll.p_y - (p->buffer_constraint);
        buffer_area.r_ur.p_x = rect.r_ur.p_x + (p->buffer_constraint);
        buffer_area.r_ur.p_y = rect.r_ur.p_y + (p->buffer_constraint);
        bool legal = true;
        TiSrArea(NULL, p->plane, buffer_area, checker_helper, (ClientData)(&legal));
        if (!legal)
        {
            p->ok = false;
            return 1;
        }
    }
    return 0;
}

bool check_buff_constraint(Plane *plane, double buffer_constraint)
{
    Rect rect = {{0, 0}, {(int)chip_width, (int)chip_height}};
    Checker_package p(true, buffer_constraint, plane);
    TiSrArea(NULL, plane, rect, checker, (ClientData)(&p));
    return (p.ok);
}