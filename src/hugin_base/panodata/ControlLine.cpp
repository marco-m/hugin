// -*- c-basic-offset: 4 -*-
/** @file CPSharedStructs.h
 *
 *  @author Steven Williams <onomou@gmail.com>
 *
 *  $Id$
 *
 *  This is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* Pre-cut cookie dough
    if (whichPointsAdded == -1) {
        
    } else if (whichPointsAdded == 0) {
        
    } else if (whichPointsAdded == 1) {
        
    } else if (whichPointsAdded == 2) {
        
    }
#include <config.h>
#include "hugin1/panoinc_WX.h"
    */
    

#include "hugin_basic.h"
#include <panotools/PanoToolsInterface.h>
#include "ControlLine.h"

#ifndef PI
#define PI 3.14159
#endif

template <class T>
void ptrswap(T *first, T *second)
{
    T third = *first;
    *first = *second;
    *second = third;
}

StraightLine::StraightLine(void) : tolerance(3.e-10), selectionDistance(30), numpoints(10)
{
    label = "line"; // todo: set this upstream to avoid duplicate labeling
    whichPointsAdded = -1;
    isPointSelected = isLineSelected = isNearLine = isNearPoint = false;
    isStraight = true;
}

void StraightLine::update(Point p)
{
    double tmp, nearest = selectionDistance * selectionDistance;
    isNearPoint = isNearLine = false;
    
    tmp = p.squareDistance(start);
    if (tmp < nearest) {
        nearest = tmp;
        isNearPoint = isNearLine  = true;
        whichPointNear = 0;
    }
    tmp = p.squareDistance(mid);
    if (tmp < nearest) {
        nearest = tmp;
        isNearPoint = isNearLine  = true;
        whichPointNear = 1;
    }
    tmp = p.squareDistance(end);
    if (tmp < nearest) {
        nearest = tmp;
        isNearPoint = isNearLine  = true;
        whichPointNear = 2;
    }
    
    if (isNearPoint) { // not near point, check if near line
        nearestPointDistance = nearest;
    } else {
        if (whichPointsAdded == 0) {
            tmp = p.squareDistance(start);
        } else if (whichPointsAdded == 1) {
            tmp = abs((mid.x-start.x)*(start.y-    p.y)-(start.x-    p.x)*(mid.y-start.y))/
                 sqrt((mid.x-start.x)*(mid.x  -start.x)+(mid.y  -start.y)*(mid.y-start.y));
        } else if (whichPointsAdded == 2) {
            if (isStraight) {
                tmp = abs((end.x-start.x)*(start.y-    p.y)-(start.x-    p.x)*(end.y-start.y))/
                     sqrt((end.x-start.x)*(end.x  -start.x)+(end.y  -start.y)*(end.y-start.y));
            } else {
                tmp = abs(p.squareDistance(start)-radius);
            }
        }
        if (tmp < selectionDistance) {
            lineDistance = tmp;
            isNearLine = true;
        }
    }
}
void StraightLine::useThisLine(void) // select closest point to location, or select only this line
{
    isPointSelected = isNearPoint;
    isLineSelected = isNearLine;
    whichPointSelected = whichPointNear;
}
inline bool StraightLine::isComplete(void)
{
    if (whichPointsAdded == 2) {
        return true;
    } else {
        return false;
    }
}
void StraightLine::addPoint(Point p)
{
    if (whichPointsAdded == -1) {
        start = straightBegin = p;
        straightEnd = start;
        whichPointsAdded = 0;
        addPoint(p);
    } else if (whichPointsAdded == 0) {
        mid = straightEnd = p;
        whichPointsAdded = 1;
        isPointSelected = true;
        whichPointSelected = 1;
    } else if (whichPointsAdded == 1) {
        end = straightEnd = p;
        whichPointsAdded = 2;
        isPointSelected = true;
        whichPointSelected = 2;
    } else if (whichPointsAdded == 2) {
        end = straightEnd = p;
        isPointSelected = false;
    }
    recalculate();
}
Point StraightLine::selectedPoint(void)
{
    if (whichPointSelected == -1) {
        return Point();
    } else if (whichPointSelected == 0) {
        return start;
    } else if (whichPointSelected == 1) {
        return mid;
    } else if (whichPointSelected == 2) {
        return end;
    }
}
void StraightLine::moveSelectedPoint(Point dest)
{
    if (!isPointSelected) {
        return;
    } else {
        if (whichPointSelected == 0) {
            start = dest;
        } else if (whichPointSelected == 1) {
            mid = dest;
        } else if (whichPointSelected == 2) {
            end = dest;
        }
    }
    recalculate();
}

inline double StraightLine::getNearestPointDistance(void)
{ return nearestPointDistance; }

inline double StraightLine::getLineDistance(void)
{ return lineDistance; }

/*
void StraightLine::deleteSelectedPoint(Point)
{
    if (!isPointSelected) {
        return;
    } else {
        
    }
    recalculate();
}
*/

std::vector<Point> StraightLine::extractPathPoints(void)
{
    std::vector<Point> cpoints;
    Point dropPoint;
    if (whichPointsAdded == -1) {
        
    } else if (whichPointsAdded == 0) {
        cpoints.push_back(Point(start.x, start.y));
    } else if (whichPointsAdded == 1) {
        for( double i = 0; i <= 10; ++i ) {
            dropPoint.x = start.x + (mid.x - start.x) * i / 10;
            dropPoint.y = start.y + (mid.y - start.y) * i / 10;
            cpoints.push_back(dropPoint);
        }
    } else if (whichPointsAdded == 2) {
        double step = (thetaEnd - thetaStart) / numpoints;
        for( double i = 0; i <= 10; ++i ) {
            dropPoint.x = radius * cos( thetaStart + i*step ) + center.x;
            dropPoint.y = radius * sin( thetaStart + i*step ) + center.y;
            cpoints.push_back(dropPoint);
        }
    }
    return cpoints;
}

void StraightLine::recalculate(void)
{
    /* Fit a circle to the points set */
    numpoints = 10;
    
    if (whichPointsAdded == -1) {
        center.x = 0;
        center.y = 0;
        radius = 0;
        isStraight = true;
    } else if (whichPointsAdded == 0) {
        center = start;
        radius = 0;
        isStraight = true;
    } else if (whichPointsAdded == 1) {
        center = (start - mid) / 2;
        radius = 0;
        isStraight = true;
    } else if (whichPointsAdded == 2) {
        Matrix3 matA, matD, matE, matF;
        matA.m[0][0] = start.x;      matA.m[0][1] = start.y;      matA.m[0][2] = 1;
        matA.m[1][0] =   mid.x;      matA.m[1][1] =   mid.y;      matA.m[1][2] = 1;
        matA.m[2][0] =   end.x;      matA.m[2][1] =   end.y;      matA.m[2][2] = 1;
        double a = matA.Determinant();
        if( false ) {//abs(a) < tolerance ) { // approximately straight line
            center = (start - mid) / 2;
            radius = 0;
            isStraight = true;
        } else {
            /* Calculate center, radius */
            isStraight = false;
            matD.m[0][0] = (start.x * start.x) + (start.y * start.y);   matD.m[0][1] = start.y;   matD.m[0][2] = 1;
            matD.m[1][0] = (  mid.x *   mid.x) + (  mid.y *   mid.y);   matD.m[1][1] =   mid.y;   matD.m[1][2] = 1;
            matD.m[2][0] = (  end.x *   end.x) + (  end.y *   end.y);   matD.m[2][1] =   end.y;   matD.m[2][2] = 1;

            matE.m[0][0] = (start.x * start.x) + (start.y * start.y);   matE.m[0][1] = start.x;   matE.m[0][2] = 1;
            matE.m[1][0] = (  mid.x *   mid.x) + (  mid.y *   mid.y);   matE.m[1][1] =   mid.x;   matE.m[1][2] = 1;
            matE.m[2][0] = (  end.x *   end.x) + (  end.y *   end.y);   matE.m[2][1] =   end.x;   matE.m[2][2] = 1;

            matF.m[0][0] = (start.x * start.x) + (start.y * start.y);   matF.m[0][1] = start.x;   matF.m[0][2] = start.y;
            matF.m[1][0] = (  mid.x *   mid.x) + (  mid.y *   mid.y);   matF.m[1][1] =   mid.x;   matF.m[1][2] =   mid.y;
            matF.m[2][0] = (  end.x *   end.x) + (  end.y *   end.y);   matF.m[2][1] =   end.x;   matF.m[2][2] =   end.y;

            double d = -matD.Determinant();
            double e =  matE.Determinant();
            double f = -matF.Determinant();

            center.x = -d / ( 2 * a );
            center.y = -e / ( 2 * a );
            radius = sqrt((d*d+e*e)/(4*a*a)-f/a);
            
            /* Calculate angles and quadrant */
            int quadStart, quadMid, quadEnd;
            Point st = start - center, md = mid - center, ed = end - center;
            findQuadrant(st, quadStart);
            findQuadrant(md, quadMid  );
            findQuadrant(ed, quadEnd  );
            st.x = abs(st.x);     st.y = abs(st.y);
            md.x = abs(md.x);     md.y = abs(md.y);
            ed.x = abs(ed.x);     ed.y = abs(ed.y);
            // should this skip the quadrant checks and use atan2 ?
            thetaStart = atan(st.y/st.x);
            thetaEnd   = atan(ed.y/ed.x);
            double thetaMid = atan(md.y/md.x);
            correctAngle(thetaStart, quadStart);
            correctAngle(thetaMid,   quadMid  );
            correctAngle(thetaEnd,   quadEnd  );
            if( thetaStart < thetaMid && thetaStart < thetaEnd && thetaMid < thetaEnd ); // nothing to do
            if( thetaStart < thetaMid && thetaStart < thetaEnd && thetaMid > thetaEnd ) {thetaStart += 2 * PI;}
          //if( thetaStart < thetaMid && thetaStart > thetaEnd && thetaMid < thetaEnd ); // not possible
            if( thetaStart < thetaMid && thetaStart > thetaEnd && thetaMid > thetaEnd ) {thetaEnd += 2 * PI;}
            if( thetaStart > thetaMid && thetaStart < thetaEnd && thetaMid < thetaEnd ) {thetaEnd -= 2 * PI;}
          //if( thetaStart > thetaMid && thetaStart < thetaEnd && thetaMid > thetaEnd ); // not possible
            if( thetaStart > thetaMid && thetaStart > thetaEnd && thetaMid < thetaEnd ) {thetaEnd += 2 * PI;}
            if( thetaStart > thetaMid && thetaStart > thetaEnd && thetaMid > thetaEnd ); // nothing to do
            
            numpoints = int( 2.7 * abs(thetaEnd - thetaStart) + 3 ); // range: 3 ... 20?
        }
    }
}

void StraightLine::findQuadrant(Point pt, int &quad)
{
         if( pt.x >= 0 && pt.y >= 0 )
        quad = 0; // first
    else if( pt.x <  0 && pt.y >= 0 )
        quad = 1; // second
    else if( pt.x <  0 && pt.y <  0 )
        quad = 2; // third
    else if( pt.x >= 0 && pt.y <  0 )
        quad = 3; // fourth
}

void StraightLine::correctAngle(double &theta, int quad)
{
         if( quad == 1 ) // quadrant 2
             theta = -theta + PI;
    else if( quad == 2 ) // quadrant 3
        theta =  theta + PI;
    else if( quad == 3 ) // quadrant 4
        theta = -theta + PI * 2;
    while( theta >= 0 )
        theta -= 2 * PI;
    theta += 2 * PI;
}

/*----------------------------------------------------------------------------------------------*/

ImageLinesCollection::ImageLinesCollection(void)
{
    lines.clear();
    isLineSelected = false;
    whichLineSelected = -1;
}

void ImageLinesCollection::update(Point)
{
    bool nearPoint, nearLine;
    nearPoint = nearLine = false;
    double d = DBL_MAX;
    
    std::vector<StraightLine>::iterator it, nearest;
    for (it = lines.begin(); it != lines.end(); ++it) { // check if mouse near a point in this loop
        if (it->isNearPoint && it->getNearestPointDistance() < d) {
            d = it->getNearestPointDistance();
            nearest = it;
            nearPoint = true;
        }
    }
    if (nearPoint) {
        nearest->useThisLine();
    } else {
        for (it = lines.begin(); it != lines.end(); ++it) { // check if mouse near a line in this loop
            if (it->isNearLine && it->getLineDistance() < d) {
                d = it->getLineDistance();
                nearest = it;
                nearLine = true;
            }
        }
        if (nearLine) {
            nearest->useThisLine();
        }
    }
}

void ImageLinesCollection::addLine(StraightLine l)
{ lines.push_back(l); }
bool ImageLinesCollection::select(int index)
{
    selectNone();
    lines[index].overrideLineSelected = true;
    lines[index].isLineSelected = true;
}
void ImageLinesCollection::selectNone(void)
{
    std::vector<StraightLine>::iterator it;
    for (it = lines.begin(); it != lines.end(); ++it) {
        it->isLineSelected = false;
        it->overrideLineSelected = false;
    }
}

bool ImageLinesCollection::deleteByIndex(unsigned int index)
{
    if (index >= lines.size()) {
        return false;
    } else {
        lines.erase(lines.begin()+index);
        return true;
    }
}

inline std::vector<Point> ImageLinesCollection::extractPathPoints(unsigned int index)
{ return lines[index].extractPathPoints(); }

/*----------------------------------------------------------------------------------------------*/

ImageLinesPair::ImageLinesPair(int fNr, int sNr)
{
    isPairSelected = false;
    firstNr = fNr;
    secondNr = sNr;
}

ImageLinesPair::ImageLinesPair(int fNr, int sNr, ImageLinesCollection f, ImageLinesCollection s)
{
    isPairSelected = false;
     first = f;      firstNr = fNr;
    second = s;     secondNr = sNr;
}

void ImageLinesPair::addLinePair(StraightLine f, StraightLine s)
{
    f.overrideLineSelected = false;
    s.overrideLineSelected = false;
     first.addLine(f);
    second.addLine(s);
}

bool ImageLinesPair::extractSelectedPathPoints(std::vector<Point> &fpts, std::vector<Point> &spts)
{
    if (isPairSelected) {
        std::vector<Point> firstpts, secondpts;
        fpts.clear();
        spts.clear();
        firstpts  =  first.extractPathPoints(selectedIndex);
        secondpts = second.extractPathPoints(selectedIndex);
        // fixme: why doesn't this work? No ControlPoint objects will compile in this file
        //for (int i = 0; i < firstpts.size(); ++i) {
        //    pts.push_back(ControlPoint( firstNr,  firstpts[i].x,  firstpts[i].y,
        //                               secondNr, secondpts[i].x, secondpts[i].y));
       // }
        fpts = firstpts;
        spts = secondpts;
        return true;
    } else {
        return false;
    }
}

bool ImageLinesPair::selectByIndex(int index)
{
    if (index >= first.lines.size()) {
        return false;
    } else {
         first.select(index);
        second.select(index);
        isPairSelected = true;
        return true;
    }
}

void ImageLinesPair::selectNone(void)
{
     first.selectNone();
    second.selectNone();
}

bool ImageLinesPair::deleteByIndex(int index)
{
    if (index >= first.lines.size()) {
        return false;
    } else {
        if (first.deleteByIndex(index) && second.deleteByIndex(index)) {
            isPairSelected = false;
            return true;
        } else {
            DEBUG_FATAL("ImageLinesCollection mismatched size! Oops!");
            return false; // not necessary if the rest of the code is clean
        }
    }
}

int  ImageLinesPair::indexOfSelected(void)
{ return selectedIndex; }

int  ImageLinesPair::getSize(void)
{
    return first.lines.size();
}

std::string ImageLinesPair::getLabel(int index)
{
    return first.lines[index].label;
}

void ImageLinesPair::setLabel(int index, std::string l)
{
     first.lines[index].label = l;
    second.lines[index].label = l;
}

bool ImageLinesPair::isWhichPair(int left,int right,ImageLinesPair*& pair)
{
    if (left == firstNr && right == secondNr) {
        pair = this;
        return true;
    } else if (right == firstNr && left == secondNr) { // swap first and second, then return
        //ImageLinesCollection *third = &first;
        //second = first;
        //first = &third;
        ptrswap(&first,&second);
        pair = this;
        return true;
    } else { // not this pair
        pair = NULL;
        return false;
    }
}

/*----------------------------------------------------------------------------------------------*/

ImageLinesPair* LineCollection::getPair(int left, int right)
{
    ImageLinesPair *foundPair;
    std::vector<ImageLinesPair>::iterator it;
    for (it = allLinesPairs.begin(); it != allLinesPairs.end(); ++it) {
        if (it->isWhichPair(left,right,foundPair) && foundPair != NULL) // pair exists
            return foundPair;
    }
    // otherwise make new pair
    ImageLinesPair p(left,right);
    allLinesPairs.push_back(p);
    allLinesPairs.back().isWhichPair(left,right,foundPair);
    return foundPair;
}
