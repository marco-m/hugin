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
    Start
    Mid
    End

Straight line outputs are:
    A list of control points
    
Straight line interactions are:
    Mouse rolls over a line
    Mouse clicks a line
    Mouse clicks a point (from line inputs)
    Deleting a point (from line inputs)
    Mouse distance from:
        The line path
        A point (from line inputs)
    
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
    Editing a specific line
        By proximity (get distance from mouse to each point on each line)
            Activate line closest to mouse
                Activate point on active line
                    Input:
                        Click to select point
                            Move selected point with mouse
                                Click to place point (deselect)
                        Keyboard arrows highlight point
                        Keyboard Return selects highlighted point
                            Keyboard arrows move point
                             Keyboard Return places point (deselect)


*/

class StraightLine
{
    public:
        /* Variables */
    
        // Note: be careful to not feed these rotated and scaled points - do those actions in the environment
        hugin_utils::FDiff2D start, mid, end;
        
        hugin_utils::FDiff2D center;
        double radius, thetaStart, thetaEnd;
        unsigned int imageNr;
        
        /* Functions */
        StraightLine(unsigned int);
        bool operator==(const StraightLine &l) const {
            return(    imageNr == l.imageNr    &&   center == l.center && radius == l.radius &&
                    thetaStart == l.thetaStart && thetaEnd == l.thetaEnd                        );
        }
        
        std::vector<ControlPoint> extractPathPoints(void);
        bool addPoint(hugin_utils::FDiff2D);
        void moveActivePoint(hugin_utils::FDiff2D);
        double getDistance(hugin_utils::FDiff2D); // sets pointNear for future use
        bool removePoint(int);
        bool removeNearPoint(hugin_utils::FDiff2D);
        void activateNearPoint(void);
        
    private:
        /* Variables */
        static const double tolerance;
        double thetaMid;
        bool isStraight;
        bool startSet, midSet, endSet;
        enum whichPoint {NONE=0, START, MID, END};
        whichPoint pointActive, pointNear;
        bool lineActive;
        std::vector<hugin_utils::FDiff2D> points; // not implemented
    
        /* Functions */
        void recalculate(void);
};

StraightLine::StraightLine(unsigned int nr):tolerance(3.e-10)
{
    imageNr = nr;
    isStraight = true;
    
      startSet = false;
        midSet = false;
        endSet = false;
    
    pointActive = 0;
    pointNear   = 0;
    
    lineActive  = false;
}

std::vector<ControlPoint> StraightLine::extractPathPoints(void)
{
    std::vector<ControlPoint> cpoints;
    
    
    //double prevx = radius * cos( thetaStart ) + center.x;
    //double prevy = radius * sin( thetaStart ) + center.y;
    hugin_utils::FDiff2D dropPoint( radius * cos( thetaStart ) + center.x,
                                    radius * sin( thetaStart ) + center.y );

    newLine.points.clear();
    cpoints.points.push_back(ControlPoint(dropPoint.x, dropPoint.y);
    for( int i = 0; i <= 10; i++ ) {
        // todo: change circle color for selected CPs
        dropPoint.x = radius * cos( thetaStart + i*step ) + center.x;
        dropPoint.y = radius * sin( thetaStart + i*step ) + center.y;
        newLine.points.push_back(applyRotInv(invScale(dropPoint)));
        //drawPoint(dc, invScale(applyRotInv(dropPoint)), 0, false);
        dc.DrawLine( hugin_utils::roundi(prevx),       hugin_utils::roundi(prevy),
                     hugin_utils::roundi(dropPoint.x), hugin_utils::roundi(dropPoint.y) );
        dc.DrawCircle(roundP(dropPoint), 5);
        prevx = dropPoint.x;
        prevy = dropPoint.y;
    }
    
    
    
    
    
    cpoints.push_back
}

bool StraightLine::addPoint(hugin_utils::FDiff2D point)
{
    if( isUnique(point) ) {
        if( startSet ) {
            if( midSet ) {
                if( endSet ) {
                    return false;
    }
    recalculate();
}

void moveActivePoint(hugin_utils::FDiff2D point)
{
    switch( pointActive )
    {
        case NONE: break;
            //return false;
        case START:
            start = point; break;
        case MID:
              mid = point; break;
        case END:
              end = point; break;
    }
    //return true;
}

double StraightLine::getDistance(hugin_utils::FDiff2D point)
{
    
    pointNear = NONE;
}

bool StraightLine::removePoint(int whichPoint)
{
    if( whichPoint < 1 || whichPoint > 3 ) {
        return false;
    } else {
        if( whichPoint == 1 ) {
          startSet = false;
        } else if( whichPoint == 2 ) {
            midSet = false;
        } else if( whichPoint == 3 ) {
            endSet = false;
        }
        return true;
    }
}

bool StraightLine::removeNearPoint(hugin_utils::FDiff2D point)
{
    getDistance( point );
    switch( pointNear )
    {
        case NONE:
            return false;
        case START:
          startSet = false; break;
        case MID:
            midSet = false; break;
        case END:
            endSet = false; break;
    }
    return true;
}

inline void StraightLine::activateNearPoint(void)
{
    pointActive = pointNear;
}

void StraightLine::recalculate(void)
{
    /* Fit a circle to the points in this structure */
    Matrix3 matA, matD, matE, matF;
    matA.m[0][0] = startx;                                     matA.m[0][1] = starty;      matA.m[0][2] = 1;
    matA.m[1][0] =   midx;                                     matA.m[1][1] =   midy;      matA.m[1][2] = 1;
    matA.m[2][0] =   endx;                                     matA.m[2][1] =   endy;      matA.m[2][2] = 1;
    double a = matA.Determinant();
    if( abs(a) < tolerance ) { // approximately straight line
        isStraight = true;
    } else {
        isStraight = false;
        matD.m[0][0] = (startx * startx) + (starty * starty);      matD.m[0][1] = starty;      matD.m[0][2] = 1;
        matD.m[1][0] = (  midx *   midx) + (  midy *   midy);      matD.m[1][1] =   midy;      matD.m[1][2] = 1;
        matD.m[2][0] = (  endx *   endx) + (  endy *   endy);      matD.m[2][1] =   endy;      matD.m[2][2] = 1;
                                                                       
        matE.m[0][0] = (startx * startx) + (starty * starty);      matE.m[0][1] = startx;      matE.m[0][2] = 1;
        matE.m[1][0] = (  midx *   midx) + (  midy *   midy);      matE.m[1][1] =   midx;      matE.m[1][2] = 1;
        matE.m[2][0] = (  endx *   endx) + (  endy *   endy);      matE.m[2][1] =   endx;      matE.m[2][2] = 1;
                                                                       
        matF.m[0][0] = (startx * startx) + (starty * starty);      matF.m[0][1] = startx;      matF.m[0][2] = starty;
        matF.m[1][0] = (  midx *   midx) + (  midy *   midy);      matF.m[1][1] =   midx;      matF.m[1][2] =   midy;
        matF.m[2][0] = (  endx *   endx) + (  endy *   endy);      matF.m[2][1] =   endx;      matF.m[2][2] =   endy;
        

        double d = -matD.Determinant();
        double e =  matE.Determinant();
        double f = -matF.Determinant();

        center.x = -d / ( 2 * a );
        center.y = -e / ( 2 * a );
        radius = hugin_utils::sqrt((d*d+e*e)/(4*a*a)-f/a);
    }
    
    
    
    findCircle(lstartx, lstarty, lmidx, lmidy, lendx, lendy, center, radius) ) {
        center = scale(center);
        radius = scale(radius);

        double arcLength = sqrt(hugin_utils::sqr(lmidx-lstartx)+hugin_utils::sqr(lmidy-lstarty)) +
                           sqrt(hugin_utils::sqr(lmidx-lendx)  +hugin_utils::sqr(lmidy-lendy)); // approximate
        // todo: make this user-selectable
        int linePointNr = int(arcLength / 100.0);
        if( linePointNr < 10 )
            linePointNr = 10;

        double stx = scale(lstartx) - center.x;
        double sty = scale(lstarty) - center.y;
        double mdx = scale(lmidx  ) - center.x;
        double mdy = scale(lmidy  ) - center.y;
        double edx = scale(lendx  ) - center.x;
        double edy = scale(lendy  ) - center.y;
        //enum Quadrant {FIRST, SECOND, THIRD, FOURTH};
        int quadStart, quadMid, quadEnd;

        findQuadrant(stx, sty, quadStart);
        findQuadrant(mdx, mdy, quadMid  );
        findQuadrant(edx, edy, quadEnd  );

        stx = abs(stx);     sty = abs(sty);
        mdx = abs(mdx);     mdy = abs(mdy);
        edx = abs(edx);     edy = abs(edy);

        // should this skip the quadrant checks and use atan2 ?
        double thetaStart = atan(sty/stx);
        double thetaMid   = atan(mdy/mdx);
        double thetaEnd   = atan(edy/edx);

        correctAngle( thetaStart, quadStart );
        correctAngle( thetaMid,   quadMid   );
        correctAngle( thetaEnd,   quadEnd   );

        if( thetaStart < thetaMid && thetaStart < thetaEnd && thetaMid < thetaEnd ); // nothing to do
        if( thetaStart < thetaMid && thetaStart < thetaEnd && thetaMid > thetaEnd ) {thetaStart += 2 * 3.14159;}
      //if( thetaStart < thetaMid && thetaStart > thetaEnd && thetaMid < thetaEnd ); // not possible
        if( thetaStart < thetaMid && thetaStart > thetaEnd && thetaMid > thetaEnd ) {thetaEnd += 2 * 3.14159;}
        if( thetaStart > thetaMid && thetaStart < thetaEnd && thetaMid < thetaEnd ) {thetaEnd -= 2 * 3.14159;}
      //if( thetaStart > thetaMid && thetaStart < thetaEnd && thetaMid > thetaEnd ); // not possible
        if( thetaStart > thetaMid && thetaStart > thetaEnd && thetaMid < thetaEnd ) {thetaEnd += 2 * 3.14159;}
        if( thetaStart > thetaMid && thetaStart > thetaEnd && thetaMid > thetaEnd ); // nothing to do

        double thetaSpan = thetaEnd - thetaStart;
        double step = thetaSpan / 10; // todo: make this adaptive and user-selectable

}

void CPImageCtrl::findQuadrant(double x, double y, int &quad)
{
         if( x >= 0 && y >= 0 )
        quad = 0; // first
    else if( x <  0 && y >= 0 )
        quad = 1; // second
    else if( x <  0 && y <  0 )
        quad = 2; // third
    else if( x >= 0 && y <  0 )
        quad = 3; // fourth
}
void CPImageCtrl::correctAngle(double &theta, int quad)
{
         if( quad == 1 ) // quadrant 2
             theta = -theta + 3.14159;
    else if( quad == 2 ) // quadrant 3
        theta =  theta + 3.14159;
    else if( quad == 3 ) // quadrant 4
        theta = -theta + 3.14159 * 2;
    while( theta >= 0 )
        theta -= 2 * 3.14159;
    theta += 2 * 3.14159;
}

bool CPImageCtrl::findCircle(double startx, double starty, double midx, double midy, double endx, double endy, FDiff2D &center, double &radius)
{
    Matrix3 matA, matD, matE, matF;
    matA.m[0][0] = startx;                                     matA.m[0][1] = starty;      matA.m[0][2] = 1;
    matA.m[1][0] =   midx;                                     matA.m[1][1] =   midy;      matA.m[1][2] = 1;
    matA.m[2][0] =   endx;                                     matA.m[2][1] =   endy;      matA.m[2][2] = 1;
                                                                   
    matD.m[0][0] = (startx * startx) + (starty * starty);      matD.m[0][1] = starty;      matD.m[0][2] = 1;
    matD.m[1][0] = (  midx *   midx) + (  midy *   midy);      matD.m[1][1] =   midy;      matD.m[1][2] = 1;
    matD.m[2][0] = (  endx *   endx) + (  endy *   endy);      matD.m[2][1] =   endy;      matD.m[2][2] = 1;
                                                                   
    matE.m[0][0] = (startx * startx) + (starty * starty);      matE.m[0][1] = startx;      matE.m[0][2] = 1;
    matE.m[1][0] = (  midx *   midx) + (  midy *   midy);      matE.m[1][1] =   midx;      matE.m[1][2] = 1;
    matE.m[2][0] = (  endx *   endx) + (  endy *   endy);      matE.m[2][1] =   endx;      matE.m[2][2] = 1;
                                                                   
    matF.m[0][0] = (startx * startx) + (starty * starty);      matF.m[0][1] = startx;      matF.m[0][2] = starty;
    matF.m[1][0] = (  midx *   midx) + (  midy *   midy);      matF.m[1][1] =   midx;      matF.m[1][2] =   midy;
    matF.m[2][0] = (  endx *   endx) + (  endy *   endy);      matF.m[2][1] =   endx;      matF.m[2][2] =   endy;
    
    double a = matA.Determinant();
    //if( a == 0 )
    //    return false;

    double d = -matD.Determinant();
    double e =  matE.Determinant();
    double f = -matF.Determinant();

    center.x = -d / ( 2 * a );
    center.y = -e / ( 2 * a );
    radius = hugin_utils::sqrt((d*d+e*e)/(4*a*a)-f/a);
    return true;
}

bool CPImageCtrl::isCollinear(StraightLine l)
{
    // todo: add tolerance for nearly-straight lines
    if ((l.start.x == l.mid.x && l.start.y == l.mid.y)||
        (l.start.x == l.end.x && l.start.y == l.end.y)||
        (  l.mid.x == l.end.x &&   l.mid.x == l.end.y))
        return true;

    if( determinant(l.start.x, l.start.y, 1,
                      l.mid.x,   l.mid.y, 1,
                      l.end.x,   l.end.y, 1) == 0 )
        return true;

    double slope1, slope2;

    slope1 = double(l.mid.y-l.start.y)/double(l.mid.x-l.start.x);
    slope2 = double(l.mid.y - l.end.y)/double(l.mid.x - l.end.x);
    if( slope1 == slope2 )
        return true;
    else
        return false;
}

class LineCollection
{
    public:
        bool addLine(StraightLine);
        bool removeLine(int);
        int findLine(hugin_utils::FDiff2D);
    private:
        std::vector<StraightLine> allLines;
}
