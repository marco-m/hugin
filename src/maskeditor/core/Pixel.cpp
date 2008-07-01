#include "Pixel.h"

bool operator<(const PixelColor &pa, const PixelColor &pb)
{
    if(pa.r == pb.r) {
        if(pa.g == pb.g)
            return pa.b < pb.b;
        else 
            return pa.g < pb.g;
    }
    return pa.r < pb.r;
}

bool operator<(const Pixel &pa, const Pixel &pb)
{
    if(pa.coord == pb.coord) {
        return pa.rgb < pb.rgb;
    } else if(pa.coord.y == pb.coord.y)
        return pa.coord.x < pb.coord.x;
    return pa.coord.y < pb.coord.y;
}

