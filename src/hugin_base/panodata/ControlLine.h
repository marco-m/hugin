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



/* Notes to self regarding straight lines

Straight lines in Hugin are used for two things:
    Matching control points (and eventually being a separate class for image alignment)
    Calibrating a lens
    
Straight line inputs are line points:
    Start = OK
    Mid   = OK
    End   = OK

Straight line outputs are:
    A list of control points = extractPathPoints()
    
Straight line interactions are:
    Mouse rolls over a line = 
    Mouse clicks a line
    Mouse clicks a Point (from line inputs)
    Deleting a Point (from line inputs)
    Mouse distance from:
        The line path
        A Point (from line inputs)
    
Automated processes:
    Find edges near line
    Fine-tune line using:
        Edges
        Other feature detection
        

Overall straight line control:
    Adding a new line
        
    Deleting an existing line
    
    Selecting a specific line (activating)
        By index (from a list)
        By proximity (get distance from mouse to each line)
    Deselecting all lines
    Editing a specific line
        By proximity (get distance from mouse to each Point on each line)
            Activate line closest to mouse
                Activate Point on active line
                    Input:
                        Click to select Point
                            Move selected Point with mouse
                                Click to place Point (deselect)
                        Keyboard arrows highlight Point
                        Keyboard Return selects highlighted Point
                            Keyboard arrows move Point
                             Keyboard Return places Point (deselect)

External function for dealing with lines:
    Draw line
        One   Point  set
        Two   points set
        Three points set
        
        Selected (activated)

*/

#include <utility>
#include <vector>

#ifndef STRAIGHTLINE
#define STRAIGHTLINE
typedef hugin_utils::FDiff2D Point;
class StraightLine
{
    public:
        enum whichPoint {NONE=0, START, MID, END};
        /* Variables */
    
        // Note: input unrotated, unscaled coordinates
        Point start, mid, end;
        Point center;
        double radius, thetaStart, thetaEnd;
        bool isStraight, isOverLine;
        unsigned int imageNr;   // associate image - does this change with image addition/deletion?
        int numpoints; // number of points to return along path
        double selectionDistance;
        
        bool lineSelected, pointIsSelected;
        whichPoint pointSelected, isOverPoint;
        Point selectedPoint;
        
        /* Functions */
        StraightLine(unsigned int=UINT_MAX); // constructor
        bool operator==(const StraightLine &l) const {
            return(    imageNr == l.imageNr    &&   center == l.center && radius == l.radius &&
                    thetaStart == l.thetaStart && thetaEnd == l.thetaEnd                        );
        } // boolean equality
        
        std::vector<hugin_utils::FDiff2D> extractPathPoints(void);
        
        /* Editing functions */
        bool addPoint(Point);
        void moveActivePoint(Point); // for moving a point on a complete line
        void moveLastPoint(Point);   // for moving a point on a new line
        //void moveLine(Point);
        
        double getNearestPointDistance(Point destination);
        double getLineDistance(Point destination);
        //double getDistance(Point); // sets pointNear for future use
        //bool removePoint(int); // remove point by index: 1 = start, etc.
        bool removeLastPoint(void);
        bool removeNearPoint(Point);
        void selectLastNearPoint(void);
        void deselectPoint(void);
        void    selectLine(void);
        void  deselectLine(void);
        //void update(Point);
    private:
        /* Variables */
        double tolerance;
        bool startSet, midSet, endSet;

        whichPoint pointNear, pointsAdded;

        std::vector<Point> points; // not implemented

        /* Functions */
        void recalculate(void);
        void findQuadrant(Point, int&);
        void correctAngle(double&, int);
};

class LineCollection // allLines
{
    public:
        LineCollection(void);
        struct linePair {StraightLine first, second;};
        //typedef std::pair<StraightLine,StraightLine> Pair;
        bool addPair(linePair);
        bool addPair(StraightLine,StraightLine);
        bool removePair(int); // removes line by index
        bool removePair(int,int,int); // removes line by index in set returned from extractLines()
        int  findLine(Point,int,int); // returns line index nearest given Point
        bool selectNearest(Point,int,int); // activates nearest line, returns true if one near enough
        bool selectLine(int,int,int); // activates indexed line in list of lines from src to dest
        void swapAll(void);
        void moveActiveLine(Point); // !NEEDS LEFT,RIGHT definition! - moves active line to destination
        // todo: instead of using update(), how about mouseEnters() and mouseLeaves()
        //        to avoid calling extractLines() so often?
        void update(Point,int,int); // check all lines and points against position - access by entering position, imageNr mouse is in, imageNr of paired image
        std::vector<StraightLine> extractLines(int,int); // returns lines in src that are in dest
        void deselectAll(void);
    private:
        //std::pair<StraightLine,StraightLine> linePair;
        //StraightLine *selectedLine // must use std::list<linePair> for this to work...
        std::vector<linePair> allLines;
        int selectedLine;
        //std::vector<linePair> allLines;
        double selectionDistance;
    
        std::vector<StraightLine*> extractLinesPointer(int,int);
};
#endif
