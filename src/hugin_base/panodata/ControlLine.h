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
 
#include <utility>
#include <vector>

#ifndef STRAIGHTLINE
#define STRAIGHTLINE
typedef hugin_utils::FDiff2D Point;

class StraightLine
{
    public:
        /* Variables */
        std::string label;
        bool isPointSelected, isLineSelected, overrideLineSelected;
        int whichPointSelected; // 0 for start, etc.
    
        bool isNearPoint, isNearLine;
        int whichPointsAdded; // -1 for none, 0 for start, etc.
        
        //Point points[3]; // maybe todo: switch to this for easier coding...?
        Point straightBegin, straightEnd; // for if the line is straight
    
        Point start, mid, end, center;    // for if the line is curved
        double radius, thetaStart, thetaEnd;
        bool isStraight;
    
        /* Functions */
        StraightLine(void);
    //    StraightLine(Point[],int); // todo for adding existing line
        bool operator==(const StraightLine &l) const {
            return(     center == l.center     && radius == l.radius &&
                    thetaStart == l.thetaStart && thetaEnd == l.thetaEnd                        );
        } // boolean equality
    
        void update(Point); // call this first - updates current near point, etc.
                            //    this may benefit from a boolean parameter to (not?)activate the line...
        void useThisLine(void); // confirms nearest point/line as selected point/line
        bool isComplete(void);
        void addPoint(Point);
        Point selectedPoint(void);
        void moveSelectedPont(Point); // moves selected point to destination
        //void deleteSelectedPoint(Point); // todo: later. Maybe.
        double getNearestPointDistance(void);
        double getLineDistance(void);
        std::vector<Point> extractPathPoints(void);
    private:
        /* Variables */
        double tolerance, selectionDistance;
        int numpoints;
    
        int whichPointNear;
        double nearestPointDistance, lineDistance;
    
        /* Functions */
        void recalculate(void);
        void findQuadrant(Point, int&);
        void correctAngle(double&, int);
};

/*----------------------------------------------------------------------------------------------*/

class ImageLinesCollection //: protected ImageLinesPair
{
    public:
        std::vector<StraightLine> lines;
        bool isLineSelected;
        int whichLineSelected;
    
        ImageLinesCollection(void);
        void update(Point);
    //protected:
        
        void addLine(StraightLine);
        bool select(int);
        void selectNone(void);
        bool deleteByIndex(unsigned int);
        std::vector<Point> extractPathPoints(unsigned int);
    
    private:
        
};

/*----------------------------------------------------------------------------------------------*/

class ImageLinesPair //: protected ImageLinesCollection
{
    public:
        /* Variables */
        bool isPairSelected;
        ImageLinesCollection first, second;
        
        /* Functions */
        ImageLinesPair(int,int);
        ImageLinesPair(int,int,ImageLinesCollection,ImageLinesCollection);
    
        void addLinePair(StraightLine,StraightLine);
        //bool extractSelectedPathPoints(std::vector<ControlPoint> &);
        bool extractSelectedPathPoints(std::vector<Point> &, std::vector<Point> &);
    
    //protected: //?
        bool selectByIndex(int); // select line in each ImageLinesCollection
        void selectNone(void);
        bool deleteByIndex(int);
        int  indexOfSelected(void);
        int  getSize(void);
        std::string getLabel(int);
        void setLabel(int, std::string);
    
        
        
        bool isWhichPair(int,int,ImageLinesPair*); // todo: name this better
                                                   // assigns this class if it is the pair, otherwise returns false
    private:
        unsigned int firstNr, secondNr, selectedIndex;
};

/*----------------------------------------------------------------------------------------------*/

class LineCollection
{
    public:
        ImageLinesPair* getPair(int,int);    // gets (creates if necessary) ImageLinesPair of leftImageNr and rightImageNr
    
    private:
        std::vector<ImageLinesPair> allLinesPairs;
};
#endif
