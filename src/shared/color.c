#include "const.h"

t_color* create_color(double r, double g, double b)
{
    t_color* pt = NULL;

    while(pt == NULL) pt = (t_color*)malloc(sizeof(t_color));

    pt->b = b;
    pt->g = g;
    pt->r = r;

    return pt;
}

t_color* create_color_from_id(int id)
{
    if(id >= MAX_NB_COLOR || id < 0) return NULL;

    if(id == 0) return create_color(255.0/255.0, 0.0, 0.0); // RED
    if(id == 1) return create_color(0, 255.0/255.0, 0.0); // GREEN
    if(id == 2) return create_color(0.0, 0.0, 255.0/255.0); // BLUE
    if(id == 3) return create_color(255.0/255.0, 255.0/255.0, 0.0); // YELLOW
    if(id == 4) return create_color(255.0/255.0, 0.0, 255.0/255.0); // PINK
    if(id == 5) return create_color(255.0/255.0, 165.0/255.0, 0.0); // ORANGE
    if(id == 6) return create_color(131.0/255.0, 131.0/255.0, 131.0/255.0); // GREY
    if(id == 7) return create_color(139.0/255.0, 69.0/255.0, 19.0/255.0); // BROWN
    if(id == 8) return create_color(132.0/255.0, 112.0/255.0, 255.0/255.0); // PURPLE
    //if(id == 9) return create_color(255.0/255.0, 255.0/255.0, 255.0/255.0); // WHITE

    return NULL;
}


void delete_color(t_color* color)
{
    free(color);
}

gboolean is_same_color(t_color* color1, t_color* color2)
{
    if(color1->b == color2->b && color1->g == color2->g && color1->r == color2->r) return TRUE;
    else return FALSE;
}

t_color* copy_color(t_color* color)
{
    return create_color(color->r, color->g, color->b);
}
