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
#include "hugin_basic.h"
#include <panotools/PanoToolsInterface.h>
#include "ControlLine.h"

#ifndef PI
#define PI 3.14159
#endif
#define Point hugin_utils::FDiff2D

StraightLine::StraightLine(unsigned int nr) : tolerance(3.e-10), selectionDistance(30)
{
    imageNr = nr;
    isStraight = true;
    isOverLine = false;
    
      startSet = false;
        midSet = false;
        endSet = false;
    
    pointSelected = NONE;
    pointNear     = NONE;
    
    lineSelected  = false;
    
    numpoints = 10;
}

std::vector<Point> StraightLine::extractPathPoints(void)
{
    std::vector<Point> cpoints;
    Point dropPoint;
    
    switch( pointsAdded )
    {
        case NONE:
            break;
        case START:
            cpoints.push_back(Point(start.x, start.y));
            break;
        case MID:
            for( double i = 0; i <= 10; i++ ) {
                dropPoint.x = start.x + (mid.x - start.x) * i / 10;
                dropPoint.y = start.y + (mid.y - start.y) * i / 10;
                cpoints.push_back(dropPoint);
            }
            break;
        case END:
            double step = (thetaEnd - thetaStart) / numpoints;
            for( double i = 0; i <= 10; i++ ) {
                dropPoint.x = radius * cos( thetaStart + i*step ) + center.x;
                dropPoint.y = radius * sin( thetaStart + i*step ) + center.y;
                cpoints.push_back(dropPoint);
            }
            break;
    }
    
    return cpoints;
}

bool StraightLine::addPoint(Point point)
{
    // todo:
    //if( !isUnique(point) )
    //    return false;
    switch( pointsAdded )
    {
        case NONE:  start = point; pointsAdded = START; break;
        case START:   mid = point; pointsAdded = MID;   break;
        case MID:     end = point; pointsAdded = END;   break;
        case END:                  return false;
    }
    recalculate();
    return true;
}

void StraightLine::moveActivePoint(Point destination)
{
    switch( pointSelected )
    {
        case NONE:                       break;
        case START: start = destination; break;
        case MID:     mid = destination; break;
        case END:     end = destination; break;
    }
}

double StraightLine::getNearestPointDistance(Point point)
{
    double nearest, tmp;
    pointNear = START;
    nearest = point.squareDistance(start);
    tmp = point.squareDistance(mid);
    if( tmp < nearest ) {
        nearest = tmp;
        pointNear = MID;
    }
    tmp = point.squareDistance(end);
    if( tmp < nearest ) {
        nearest = tmp;
        pointNear = END;
    }
    if( selectionDistance * selectionDistance < nearest )
        pointNear = NONE;
    return nearest;
}

double StraightLine::getLineDistance(Point point)
{
    switch( pointsAdded )
    {
        case NONE:  return 0;
        case START: return point.squareDistance(start);
        case MID:   return abs((mid.x-start.x)*(start.y-point.y)-(start.x-point.x)*(mid.y-start.y))/
                            sqrt((mid.x-start.x)*(mid.x-start.x)+(mid.y-start.y)*(mid.y-start.y));
        case END:   return abs(point.squareDistance(start) - radius);
    }
}
/*
bool StraightLine::removePoint(int point)
{
    if( point < 1 || point > 3 ) {
        return false;
    } else {
        if( point == 1 ) {
          startSet = false;
        } else if( point == 2 ) {
            midSet = false;
        } else if( point == 3 ) {
            endSet = false;
        }
        return true;
    }
}
*/

bool StraightLine::removeLastPoint(void)
{
    switch(pointsAdded)
    {
        case NONE:                       return false;
        case START: pointsAdded = NONE;  return true;
        case MID:   pointsAdded = START; return true;
        case END:   pointsAdded = MID;   return true;
    }
    
}

// todo: the current structure does not allow deleting the middle point intuitively
//         ... perhaps reconfigure to use startSet, midSet, endSet instead of pointsAdded?
bool StraightLine::removeNearPoint(Point point)
{
    if( pointsAdded != END )
        return false;
    getNearestPointDistance( point );
    switch( pointNear )
    {
        case NONE:
            return false;
        case START:
            start = end;
            removeLastPoint();
            return true;
        case MID:
            start = end;
            pointsAdded = MID;
            return true;
        case END:
            pointsAdded = MID;
            return true;
    }
}

inline void StraightLine::selectLastNearPoint(void)
{
    pointSelected = pointNear;
    //if( pointSelected != NONE )?
        lineSelected = true;
}

inline void StraightLine::deselectPoint(void)
{ pointSelected = NONE; }

inline void StraightLine::selectLine(void)
{ lineSelected = true; }

inline void StraightLine::deselectLine(void)
{ lineSelected = false; }

/*
void StraightLine::update(Point location)
{
    double ldist = getLineDistance(location);
    if( dist < selectionDistance ) {
        isOverLine = true;
        double pdist = getNearestPointDistance(location);
        if( pdist < selectionDistance ) {
            isOverPoint = pointNear; // probably redundant use of pointNear
        }
    } else {
        isOverLine = false;
        isOverPoint = NONE;
    }
}
*/

void StraightLine::recalculate(void)
{
    /* Fit a circle to the points set */
    numpoints = 10;
    switch( pointsAdded )
    {
        case NONE:
            center.x = 0;   center.y = 0;
            radius = 0;
            isStraight = true;
            break;
        case START:
            center = start;
            radius = 0;
            isStraight = true;
            break;
        case MID:
            center = (start - mid) / 2;
            radius = 0;
            isStraight = true;
            break;
        case END:
            Matrix3 matA, matD, matE, matF;
            matA.m[0][0] = start.x;      matA.m[0][1] = start.y;      matA.m[0][2] = 1;
            matA.m[1][0] =   mid.x;      matA.m[1][1] =   mid.y;      matA.m[1][2] = 1;
            matA.m[2][0] =   end.x;      matA.m[2][1] =   end.y;      matA.m[2][2] = 1;
            double a = matA.Determinant();
            if( abs(a) < tolerance ) { // approximately straight line
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
            break;
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

/* ------------------------------------------------------------- */

LineCollection::LineCollection(void)
{
    selectionDistance = 30;
}

bool LineCollection::addPair(linePair p)
{
    // should check if line is a duplicate?
    allLines.push_back(p);
    return true;
}

bool LineCollection::addPair(StraightLine l1, StraightLine l2)
{
    // should check if line is a duplicate?
    linePair lp;
    lp.first = l1;
    lp.second = l2;
    allLines.push_back(lp);
    return true;
}

bool LineCollection::removePair(int index)
{
    // should check if line exists?
    allLines.erase(allLines.begin() + index);
    return true;
}

bool LineCollection::removePair(int index, int src, int dest)
{
    int i = 0;
    while( i <= index ) {
        if( allLines[i].first.imageNr == src && allLines[i].second.imageNr == dest )
            i++;
        else if( allLines[i].second.imageNr == src && allLines[i].first.imageNr == dest )
            i++;
    }
    i--;
    allLines.erase(allLines.begin() + i);
}

int LineCollection::findLine(Point point, int src, int dest)
{
    selectNearest(point);
    return selectedLine;
}

bool LineCollection::selectNearest(Point point, int src, int dest)
{
    std::vector<StraightLine*> lines(extractLinesPointer(src,dest));
    if( lines.size() == 0 )
        return false;
    int index = 0;
    double shortest = lines[0]->getLineDistance(point);
    double tmp;
    for( int i = 1; i < lines.size(); i++ ) {
        tmp = lines[i]->getLineDistance(point);
        if( tmp < shortest ) {
            shortest = tmp;
            index = i;
        }
    }
    if( shortest < selectionDistance )
    {
        selectedLine = index;
        lines[index]->selectLastNearPoint();
        return true;
    }
}

void LineCollection::moveActiveLine(Point destination)
{
    if( selectedLine < 0 )
        return;
    allLines[selectedLine].moveActivePoint(destination);
}

void LineCollection::update(Point location, int src, int dest)
{
    deselectAll();
    double dist;
    int nearestIndex = -1;
    std::vector<StraightLine*> lines(extractLinesPointer(src,dest));
    double nearestDist = selectionDistance;
    for( int i = 0; i < lines.size(); i++ ) {
        dist = lines[i]->getNearestPointDistance(location);
        if( dist < nearestDist ) {
            nearestIndex = i;
            nearestDist = selectionDistance;
        }
    }
    if( nearestIndex < 0 ) { // no points close enough
        for( int i = 0; i < lines.size(); i++ ) {
            dist = lines[i]->getLineDistance(location);
            if( dist < nearestDist ) {
                nearestIndex = i;
                nearestDist = selectionDistance;
            }
        }
        if( nearestIndex < 0 ) { // no line close enough
            return;
        } else {
            selectedLine = nearestIndex;
            lines[selectedLine]->selectLine();
        }
    } else {
        selectedLine = nearestIndex;
        lines[selectedLine]->selectLastNearPoint();
    }
}

std::vector<StraightLine> LineCollection::extractLines(int src, int dest)
{
    std::vector<StraightLine> lines;
    for( int i = 0; i < allLines.size(); i++ ) {
        if( allLines[i].first.imageNr == src && allLines[i].second.imageNr == dest )
            lines.push_back(allLines[i].first);
        else if( allLines[i].second.imageNr == src && allLines[i].first.imageNr == dest )
            lines.push_back(allLines[i].second);
    }
    return lines;
}

void LineCollection::deselectAll(void)
{
    for( int i = 0; i < allLines.size(); i++ ) {
        allLines[i].deselectLine();
    }
}

std::vector<StraightLine*> LineCollection::extractLinesPointer(int src, int dest)
{
    std::vector<StraightLine*> lines;
    for( int i = 0; i < allLines.size(); i++ ) {
        if( allLines[i].first.imageNr == src && allLines[i].second.imageNr == dest )
            lines.push_back(&(allLines[i].first));
        else if( allLines[i].second.imageNr == src && allLines[i].first.imageNr == dest )
            lines.push_back(&(allLines[i].second));
    }
    return lines;
}
