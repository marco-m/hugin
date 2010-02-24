
/**
 * Scaling for multiresolution processing.
 * Copyright (C) 2009  Lukáš Jirkovský <l.jirkovsky@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 *Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <vigra/diff2d.hxx>

using namespace vigra;

bool multires_scale(int origWidth, int origHeight, int num_iterations, int iteration, Size2D &newSize)
{
    // compute width
    int resized_width = origWidth / ( num_iterations/(iteration+1) );
    //compute height
    int resized_height = origHeight / ( num_iterations/(iteration+1) );
    // it's not worthy to scale to less than 100px per side
    if (resized_width > 100 && resized_height > 100) {
        // return computed size
        newSize = Size2D(resized_width, resized_height);
        return true;
    } else if (origWidth >= 100 && origHeight >= 100) {
        // resize it to the smallest value (ie 100px for the shorter side)
        if (origWidth >= origHeight) {
            newSize = Size2D(100*origWidth/origHeight, 100);
            return true;
        } else {
            newSize = Size2D(100, 100*origHeight/origWidth);
            return true;
        }
    } else {
        // don't scale at all
        // just copy weights as if no scaling seting was applied
        return 0;
    }
}

bool multires_scale(Size2D originalSize, int num_iterations, int iteration, Size2D &newSize)
{
    return multires_scale(originalSize.width(), originalSize.height(), num_iterations, iteration, newSize);
}
