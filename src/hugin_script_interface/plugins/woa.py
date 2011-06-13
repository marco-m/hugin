#! /usr/bin/env python

from __future__ import print_function

gpl = r"""
    woa.py - warped overlap analysis
    generate control points from warped overlap images
    
    Copyright (C) 2011  Kay F. Jahnke

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
"""

# note that if you want to read the script, it's written bottom-up, so the
# higher-level routines are towards the end.

import os
import sys
import hsi
import re
import copy
import math
import itertools
import subprocess

# our very own exception:

class RunTimeError ( Exception ) :

    def __init__ ( self , text ) :
        self.text = text

    def __str__ ( self ) :
        return self.text
    
# we keep a global variable for arguments - if called from the command
# line the argument parser will overwrite it, but since a plugin
# can't receive arguments, we use a different approach then: if there
# is a file 'woa.ini' in the user's home directory, the arguments are read
# from it; otherwise sensible defaults are used.
# Since the mechanism is only there to bridge the time until a GUI for
# plugins becomes reality, it's sloppily programmed and does not check
# the woa.ini file very much.
# hugin's pwd is /tmp, which is no good location to look for woa.ini.
# That's why it's expected in the user's home directory.

class argobj :

    def __init__ ( self , inifile = '~/woa.ini' ) :

        self.focus = None

        # if woa.ini can be found, read arguments from it
        
        path = os.path.expanduser ( inifile )
        if os.path.exists ( path ) :

            import ConfigParser
            f = open ( path , 'r' )
            config = ConfigParser.ConfigParser()
            config.readfp ( f )

            self.basename = config.get ( 'woa arguments' , 'basename' )
            self.ceiling = config.getfloat ( 'woa arguments' , 'ceiling' )
            self.cpg = config.get ( 'woa arguments' , 'cpg' )
            self.margin = config.getint ( 'woa arguments' , 'margin' )
            self.prolific = config.getboolean ( 'woa arguments' , 'prolific' )
            self.scale = config.getfloat ( 'woa arguments' , 'scale' )
            self.threshold = config.getfloat ( 'woa arguments' , 'threshold' )
            self.verbose = config.get ( 'woa arguments' , 'verbose' )

        else :
                
            # no woa.ini - use sensible defaults
            
            self.basename = 'woa'
            self.ceiling = 1.0
            self.cpg = 'apsc' # 'cpfind'
            self.margin = 0
            self.prolific = False
            self.scale = 0.25
            self.threshold = 0.01
            self.verbose = True

# we want to use this variable globally:

args = None

# verbose-print: print only if args.verbose is set

def vpr ( *x ) :
    if args.verbose :
        print ( *x )
    
# due to differences in coordinate system for raster and vector graphics
# if the mask coincides with the left image margin, I have to extend it 
# by one pixel. I have christened

mask_offset = -1.0

# when comparing positions, if they differ by at most POSITION_DELTA,
# they will be considered equal

POSITION_DELTA = 0.000001

# I'm using a cartesian/polar coordinate transform modified so
# it works with hugin's lon/lat system and degrees. The transform is
# into a right handed system with (0,0) transforming to (1,0,0), z up

def cartesian3_to_polar2 ( p3 ) :

    azimuth = math.atan2 ( p3[1] , p3[0] )
    inclination = math.acos ( p3[2] )

    return ( math.degrees ( azimuth ) ,
             90.0 - math.degrees ( inclination ) )

# and back

def polar2_to_cartesian3 ( p2 ) :

    azimuth = math.radians ( p2.x )
    inclination = math.radians ( 90.0 - p2.y )
    x = math.sin ( inclination ) * math.cos ( azimuth )
    y = math.sin ( inclination ) * math.sin ( azimuth )
    z = math.cos ( inclination )
    
    return ( x , y , z )

def normalize ( cart ) :

    sqmag = cart[0] * cart[0] + cart[1] * cart[1] + cart[2] * cart[2]
    mag = math.sqrt ( sqmag )
    rv = ( cart[0] / mag , cart[1] / mag , cart[2] / mag )
    return rv

# get_tiff_offset is needed to find the offsets in cropped TIFFS.
# This is done with a call to tiffdump, a utility program that comes
# with libtiff. I'm not entirely happy with this solution, but I found
# no more straightforward way to extract the data.
# Basially, tiffdump is called, it's output is scanned into a dictionary
# and the needed values to calculate the offsets are taken from the
# dictionary. A tuple with xoffset and yoffset in pixels is returned.
# If it's not a 'cropped TIFF' the offsets are returned as 0,0

def get_tiff_offset ( image_file ) :

    p = subprocess.Popen ( shell = True ,
                           args = 'tiffdump %s' % image_file ,
                           stdout=subprocess.PIPE )

    output=p.stdout.readlines()
    gleaned = dict()

    for tag in output :
        t = tag.strip()
        fieldname = re.match ( r'[^\(]+' , t ) . group(0) . strip()
        fieldcontent = re.match ( r'([^<]+<)([^>]+)' , t )
        if fieldcontent:
            gleaned [ fieldname ] = fieldcontent.group(2)

    xpos = float ( gleaned.get ( 'XPosition' , 0 ) )
    ypos = float ( gleaned.get ( 'YPosition' , 0 ) )
    xres = float ( gleaned.get ( 'XResolution' ) )
    yres = float ( gleaned.get ( 'YResolution' ) )

    xoff = xpos * xres
    yoff = ypos * yres

    return xoff , yoff

# some global variables to reduce overhead - we just reuse the same
# FDiff2D objects every time we call the transforms

transform_in = hsi.FDiff2D(0,0)
transform_inter = hsi.FDiff2D(0,0)
transform_out = hsi.FDiff2D(0,0)

# transform point coordinates using panotools transforms
# using the global variables above for source and target

def transform_point ( p , tf ) :
    
    transform_in.x = p.x
    transform_in.y = p.y

    if tf.transform ( transform_out , transform_in ) :
        return point ( transform_out.x , transform_out.y )

    vpr ( 'transform failed for %s' % str ( p ) )
    return None

# when transforming from one image to the other, two transforms
# are needed: img0 to pano and pano to img1

def double_transform_point ( p , tf1 , tf2  ) :

    transform_in.x = p.x
    transform_in.y = p.y

    if tf1.transform ( transform_inter , transform_in ) :
        if tf2.transform ( transform_out , transform_inter ) :
            return point ( transform_out.x , transform_out.y )

    vpr ( 'transform failed for %s' % str ( p ) )
    return None

# next is a set of stripped-down geometrical primitives that are just
# sufficient to do the maths we need for masking the overlapping regions
# of the images

class point :

    def __init__ ( self , ix , iy ) :

        self.x = ix
        self.y = iy

    def distance ( self , other ) :

        dx = other.x - self.x
        dy = other.y - self.y
        return math.sqrt ( dx * dx + dy * dy )

    def delta ( self , other ) :

        dx = other.x - self.x
        dy = other.y - self.y
        return ( dx , dy )

    def __str__ ( self ) :

        return '%s (%.5f, %.5f)' % ( id(self) , self.x , self.y )

# a segment contains it's two end points and some auxilliary data
# to facilitate mathematics on the segment. Note that our segment
# notion is planar and uses image coordinates.

class segment :

    # we keep some more state in the segment object which is needed
    # repeatedly in various places
    
    def __init__ ( self , ia , ib ) :

        self.a = ia
        self.b = ib

        self.length = ia.distance ( ib )

        if ia.x == ib.x :
            self.minx = self.maxx = ia.x
            self.dx = 0.0
        else :
            self.minx = min ( ia.x , ib.x )
            self.maxx = max ( ia.x , ib.x )
            self.dx = ib.x - ia.x
        if ia.y == ib.y :
            self.miny = self.maxy = ia.y
            self.dy = 0.0
        else :
            self.miny = min ( ia.y , ib.y )
            self.maxy = max ( ia.y , ib.y )
            self.dy = ib.y - ia.y

    def __str__ ( self ) :

        return '[ %s %s ]' % ( self.a , self.b )

    # we detect only 'true' intersections, so if the segment
    # lies fully on the horizontal/vertical, this is not considered
    # an intersection.
    
    def intersect_with_vertical ( self , x ) :

        if abs ( self.dx ) > POSITION_DELTA : 
            if self.minx <= x <= self.maxx :
                return (   self.a.y
                         + ( x - self.a.x )
                         * ( self.b.y - self.a.y )
                         / ( self.b.x - self.a.x ) )

        return None
    
    def intersect_with_horizontal ( self , y ) :

        if abs ( self.dy ) > POSITION_DELTA :
            if self.miny <= y <= self.maxy :
                return (   self.a.x
                         + ( y - self.a.y )
                         * ( self.b.x - self.a.x )
                         / ( self.b.y - self.a.y ) )

        return None
    
    # segments are used with z-values that range from 0 (at point a)
    # to 1 (at point b). Locate will yield the point corresponding to z.
    
    def locate ( self , z ) :

        assert 0.0 <= z <= 1.0

        if z == 0.0 :
            return self.a
        elif z == 1.0 :
            return self.b
        else :
            return point ( self.a.x + z * self.dx , self.a.y + z * self.dy )

    # contains tests if point p is on this segment.
    # a certain delta is deemed acceptable

    def contains ( self , p , delta = POSITION_DELTA ) :

        # our test works like this:
        # all points on the line segment satisfy L = A + z ( dx , dy )
        # with 0 <= z <= 1
        # we assume P is on L. So we calculate a z for it and test if
        # 1. 0 <= z <= 1
        # 2. P = A + z ( dx , dy )
        # if dx has greater magnitude than dy, we calculate z by way of
        # the horizontal distance, otherwise by way of the vertical distance.
        # We assume that the case where dx == dy == 0 never occurs.
        
        if abs ( self.dx ) >= abs ( self.dy ) :
            # horizontal distance between p and self.a
            dpx = p.x - self.a.x
            # in ratio to self.dx yields z
            z = dpx / self.dx
            # if z is roughly between 0 and 1, the match is possible
            if - delta <= z <= 1.0 + delta :
                # going z * dy in vertical direction from self.a
                # yields a point on the line segment
                yy = self.a.y + z * self.dy
                if abs ( yy - p.y ) < delta :
                    # if this value coincides with p.y, we've come
                    # full circle and the point must be on the segment.
                    # print ( self , 'contains' , p , z )
                    return True
        else :
            # the symmetric case, where we start out with the vertical
            # distance.
            dpy = p.y - self.a.y
            z = dpy / self.dy
            if - delta <= z <= 1.0 + delta :
                xx = self.a.x + z * self.dx
                if abs ( xx - p.x ) < delta :
                    # print ( self , 'contains' , p , z )
                    return True
    
        # print ( self , 'does not contain' , p )
        return False
        
# routine to make a list of corner points from a SrcPanoImage object.
# The point list can be used to create a polygon object.
# We also pass back left, top, right and bottom for good measure.

def img_to_point_list ( img ) :

    w = img.getWidth()
    h = img.getHeight()

    left =   - w / 2.0 - args.margin
    bottom = - h / 2.0 - args.margin
    right =    w / 2.0 + args.margin
    top =      h / 2.0 + args.margin

    return ( [ point ( left ,  top ) ,
               point (  right ,  top ) ,
               point (  right , bottom ) ,
               point ( left , bottom ) ] ,
             left , top , right , bottom )

# class polygon contains a set of segments. These segments are meant
# to form a closed polygon, but this is not checked or enforced.
# The implementation is minimal and geared towards the intended use,
# this is not library-grade code.

class polygon :

    def __init__ ( self ,
                   ipoints = [] ,
                   reverse = False ,
                   close = True ) :

        # the points may be used in reverse order
        
        if reverse :
            ipoints = [ p for p in reversed ( ipoints ) ]

        # keep number of segments as state, this has to be updated
        # if new segments are taken in. A sacrifice to efficiency
        # instead of calling len() always.
        
        self.count = len ( ipoints )

        # the polygon is closed; the last segment
        # joins last and first point

        self.segments = []
        if self.count >= 2 :
            a = ipoints[0]
            for p in ipoints[1:] :
                self.segments.append ( segment ( a , p ) )
                a = p
            if close :
                self.segments.append ( segment ( a , ipoints[0] ) )

    def __str__ ( self ) :

        result = ''
        nr = 0
        for s in self.segments :
            result += 'SEG %02d %s\n' % ( nr , s )
            nr += 1
            
        return result

    # take in a point. This point has to be on one of the segments
    # and the effect is to split the segment in two where the point is.
    # if this fails, False is retuned, otherwise True.
    
    def take_in ( self , p ) :

        new_seglist = []
        inserted = False
        for s in self.segments :
            if ( not inserted ) and s.contains ( p ) :
                inserted = True
                new_seglist.append ( segment ( s.a , p ) )
                new_seglist.append ( segment ( p , s.b ) )
            else :
                new_seglist.append ( s )
        if inserted :
            self.segments = new_seglist
            self.count = len ( new_seglist )

        return inserted

# class dot acts as a cursor (or iterator) on a polygon. It will
# sample the polygon circumference, producing sample points every
# so often, and will also stop at all corner points.

class dot :

    def __init__ ( self , ipolygon ,
                   istage = 0 , iwind = 0 ,
                   istride = 0.1 , iz = 0.0 ,
                   idelta = POSITION_DELTA ) :

        self.track = ipolygon
        self.stage = istage
        self.wind = iwind
        self.stride = istride
        self.z = iz
        self.delta = idelta
        self.current_segment = self.track.segments [ self.stage ]

    # z is the normalized distance from the beginning of the current
    # segment to the current sample location:
    
    def position ( self ) :

        return self.current_segment.locate ( self.z )

    def __str__ ( self ) :

        return 'dot track %d stage %d wind %d z %f %s' % (
                id ( self.track ) ,
                self.stage ,
                self.wind ,
                self.z ,
                self.position() )
    
    # advance to next sample location, or to next corner point
    # if that is nearer
    
    def advance ( self ) :

        self.z += self.stride / self.current_segment.length

        # if z is at (within delta) or past the end of the current
        # segment, advance to the beginning of the next segment.

        if self.z + self.delta >= 1.0 :
            self.stage += 1
            if self.stage >= self.track.count :
                self.stage = 0
                self.wind += 1
            self.current_segment = self.track.segments [ self.stage ]
            self.z = 0.0
            
    # termination criterion. If the dot is copied at the beginning
    # of a circumambulation to 'start', passed ( start ) will yield
    # True if the start point has been passed again
    
    def passed ( self , start ) :

        if self.wind > start.wind :
            if self.stage < start.stage :
                return False
            elif self.stage == start.stage :
                if self.z <= start.z :
                    return False
            return True
        return False

# shift an initial part of the outline to it's end
# so that the outline starts with an 'in' type crossover point

def in_point_to_tip ( outline ) :   

    head = []
    tail = iter ( outline )
    for p in tail :
        if p.direction == 'in' :
            outline = [ p ]
            break
        head.append ( p )
        
    outline.extend ( tail )
    outline.extend ( head ) 

    return outline

# produce a set of include and exclude masks depending on image overlap.
# This routine is central to the script, and most code up to here serves
# it. The masking of the overlapping regions is non-trivial:
# A proper mathematical description like y=f(x) is only available for
# either of the image margins in coordinates of the image itself, where
# these are trivially horizontals and verticals. When transformed to pano
# space or even to the other image's coordinates, these straight lines become
# curves, and their shape depends on many factors: image size and orientation,
# lens correction coefficients, projection types. The result of all these
# factors can be obtained for a single point at a time with some precision
# by using the panotools transforms, but this all we have. To match the
# curved lines, iterations have to be used, and for some points in the 'other'
# image there may not even exist a valid transform to coordinates of 'this'
# image. I have taken the approach to move only 'inside' 'this' image and
# avoid looking at points 'outside' apart from their status as being 'outside'.
# The resulting code does the trick, but I haven't tested all corner cases -
# I'm slightly concerned with images that have touching opposite margins
# (e.g. 360X180 equirects) - the code may break here, I'll have to test.

def mask_nonoverlaps ( pano , img0 , img1 ) :

    # for consistency throughout the script,
    # variables ending with '0' pertain to image 0,
    # which is also refered to as 'this' image
    # variables ending in '1' to image 1
    # which is also refered to as the 'other' image.
    # first we have to get the image metrics and the
    # transforms

    pano_options = pano.getOptions()

    # we create transforms from pano coordinates to image coordinates

    tf0 = hsi.Transform()
    tf0.createTransform ( img0 , pano_options )

    #tf1 = hsi.Transform()
    #tf1.createTransform ( img1 , pano_options )

    # and the reverse transform, image to pano

    #rtf0 = hsi.Transform()
    #rtf0.createInvTransform ( img0 , pano_options )

    rtf1 = hsi.Transform()
    rtf1.createInvTransform ( img1 , pano_options )

    # we also generate a polygon from 'this' image and the 'other'
    # image as well.

    points , left , top , right , bottom = img_to_point_list ( img0 )

    # later on we'll insert crosover points into this polygon with
    # direction 'in' or 'out', but the points we put in now are
    # not crosover points and have no 'direction'
    
    for p in points :
        p.direction = None

    p0 = polygon ( points )

    points , left1 , top1 , right1 , bottom1 = img_to_point_list ( img1 )
    p1 = polygon ( points )

    # we will need 'this' image's true extent and the x and y offset
    # from the center

    w0 = img0.getWidth()
    h0 = img0.getHeight()

    true_left = - w0 / 2.0
    true_bottom = - h0 / 2.0

    # the next section defines a bunch of local functions that
    # use the current metrics. This reduces parameter passing
    # and makes the code easier to read.

    # min_distance_from_margin will calculate the distance from the
    # nearest image boundary.
    
    def min_distance_from_margin ( p ) :

        return min ( abs ( p.x - left ) ,
                     abs ( right - p.x ) ,
                     abs ( p.y - bottom ) ,
                     abs ( top - p.y ) )
    
    # 'inside' criterion: True if point p is within this images's
    # boundaries. Note that here the rouine works on points in
    # 'this' image's coordinates.
    # We return the minimal distance from the margin to allow
    # for smaller strides near the edge. Negative proximity
    # signals a failed transform. Points very near the edge
    # are taken to be inside.

    def inside ( p ) :

        if not p :
            return False , -1.0
        
        if (     ( left <= p.x <= right )
             and ( bottom <= p.y <= top ) ) :
            return True , min_distance_from_margin ( p )

        md = min_distance_from_margin ( p )
        if md <= POSITION_DELTA :
            return True , 0.0
        else :
            return False , md

    # the double projection from one image to another is quite imprecise.
    # We need the crossover point to be precisely on 'this' image's boundary
    # for the inclusion in take_in to succeed.
    
    def put_on_margin ( p ) :

        x = None
        y = None

        if abs ( p.x - left ) <= 0.001 :
            x = left
            y = p.y
        elif abs ( p.x - right ) <= 0.001 :
            x = right
            y = p.y

        if abs ( p.y - top ) <= 0.001 :
            y = top
            x = p.x
        elif abs ( p.y - bottom ) <= 0.001 :
            y = bottom
            x = p.x

        if x is None or y is None :
            raise RunTimeError ( 'cannot put %s on margin' % p )

        return point ( x , y )
    
    # project_in yields a point from the other image
    # projected into this one

    def project_in ( p ) :

        return double_transform_point ( p , rtf1 , tf0 )

    # localize_crossing uses an iteration to approach the point where the
    # contours cross. This is superior to intersecting a line segment between
    # two margin points from the 'other' image. it approaches the true
    # intersection point to arbitrary precision - within the precision
    # constraints of the double panotools transform.

    def localize_crossing ( cseg ) :

        # first, find the point that is inside
        if cseg.a.inside :
            pin = cseg.a
            pout = cseg.b
            direction = 'out'
        else :
            pin = cseg.b
            pout = cseg.a
            direction = 'in'

        stride = 0.5
        delta = pin.delta ( pout )
        
        # we go on from the current position, but if we've
        # gone too far, we omit the step. Then we repeat with
        # half the step, and so on.
        
        while stride >= POSITION_DELTA :
            candidate = point ( pin.x + stride * delta[0] ,
                                pin.y + stride * delta[1] )
            twin = project_in ( candidate )
            ins , md = inside ( twin )
            if ins :
                pin = candidate
            stride /= 2.0

        # pin is now very near the actual crossing, and in coordinates
        # of the other image. We return this point as the crossing point.

        pin.direction = direction
        pin.twin = project_in ( pin )

        # we need to locate pin.twin precisely on the image boundary,
        # so that the insertion routine succeeds.
        
        pin.twin = put_on_margin ( pin.twin )
        
        vpr ( 'crossover i0:' , pin , 'i1:' , pin.twin )        
        pin.inside = True
        pin.proximity = 0.0
        return pin

    # now we're set up and start the routine proper:
    # we already have p1, a polygon from the 'other' image's margin,
    # and we sample it with standard_stride pixel steps.
    
    standard_stride = 100.0
    d = dot ( p1 , istride = standard_stride )
    outline = []
    total_coincidence = True

    # we walk the whole contour of the other image, saving each
    # point along the way, amended with additional information

    start = copy.copy ( d )
    while not d.passed ( start ) :
        # get position of point (in other image's coordinates)
        p = d.position()
        # try getting the corresponding coordinates in this image
        p.twin = project_in ( p )
        # note that p.twin may be None if the transform failed,
        # in which case inside() returns False, 0.0
        p.inside , p.proximity = inside ( p.twin )
        # if any of the points are further than POSITION_DELTA
        # away from this image's boundary, it's not a total coincidence.
        if p.proximity >= POSITION_DELTA :
            total_coincidence = False
        p.direction = None
        # here we might modify the stride depending on proximity
        # for now we just carry on with the same stride
        outline.append ( p )
        d.advance()

        if total_coincidence :
            vpr ( 'images boundaries coincide totally' )
            points , left , top , right , bottom = img_to_point_list ( img0 )
            include_mask = [ ( p.x , p.y ) for p in points ]
            img0.exclude_masks = []
            img0.include_masks = [ include_mask ]
            img0.overlap_center = point ( 0.0 , 0.0 )
            # TODO: maybe check here if rotation wouldn't be better
            img0.rotate_overlap_pano = False
            return True
            
    # now we inspect the outline for 'crossings', where one point
    # is 'inside' and the next isn't - or the other way round.
    
    crossings = []
    amended_outline = []
    previous = outline[-1]
    hop_threshold = 0.7 * min ( w0 , h0 ) # estimate only

    for current in outline :
        if current.inside != previous.inside :
            # we have found a crossing, localize it precisely
            xp = localize_crossing ( segment ( previous , current ) )
            amended_outline.append ( xp )
            crossings.append ( xp )
        elif current.inside : # both points are inside
            # check for a crossing to the other side of a very-wide-angle image
            if current.distance ( previous ) >= hop_threshold :
                raise RunTimeError ( 'hop threshold exceeded, jump to other image margin' )
                # TODO: this isn't dealt with yet, we'd have to create two
                # crossover points, one for going out and one for coming
                # back in again.
        amended_outline.append ( current )
        previous = current

    if not crossings :
        if not outline[0].inside :
            vpr ( 'other image totally outside this one' )
            # ... but it might be totally enclosing it, in which case
            # the symmetric call will deal with it.
            return False
        vpr ( 'other image totally within this one' )
        # note that the special case of total coincidence is dealt with
        # separately above.
        # we need to construct a mask with a hole.
        # this is topologically simple, but to construct a mask that
        # will work for hugin takes some trickery. I would have moved
        # this code into a separate routine, but I need the infrastructure
        # here, like the transforms and extents.

        # first, find the leftmost point and extents of
        # the 'other' image's outline.

        minx = 1000000.0
        miny = 1000000.0
        maxx = -1000000.0
        maxy = -1000000.0
        
        for p in outline :
            if p.twin.x < minx :
                leftmost = p
                minx = p.twin.x
            elif p.twin.x > maxx :
                maxx = p.twin.x
            if p.twin.y < miny :
                miny = p.twin.y
            elif p.twin.y > maxy :
                maxy = p.twin.y

        # we make a list of outline points with 'leftmost' at it's tip

        source = iter ( outline )
        head = []
        for p in source :
            if p is leftmost :
                inner_circuit = [ p ]
                break
            head.append ( p )
        inner_circuit.extend ( source )
        inner_circuit.extend ( head )

        # this happens to be the include mask, let's store it
        include_mask = [ ( p.twin.x , p.twin.y )
                         for p in inner_circuit ]

        # we add start point again at the end to use this bit
        # for the exclude mask
        
        inner_circuit.append ( leftmost )

        # for the mask, we need these points, in reverse order,
        # transformed to img0 coordinates, as tuples:
        
        inner_circuit_transformed = [ ( p.twin.x , p.twin.y )
                                      for p in reversed ( inner_circuit ) ]
                
        points , left , top , right , bottom = img_to_point_list ( img0 )
        
        # the 'mask with hole' starts with img0's corner points in
        # clockwise order, plus 'leftmost' projected to the left
        # margin
        
        exclude_mask = [ ( left , top ) ,
                         ( right , top ) ,
                         ( right , bottom ) ,
                         ( left , bottom ) ,
                         ( left , leftmost.twin.y ) ]


        # now the 'inner circuit' is appended (counterclockwise)
        # and finally the inner circuit is linked again with
        # the outline of img0

        exclude_mask.extend ( inner_circuit_transformed )
        exclude_mask.append (  ( left , leftmost.twin.y ) )

        img0.exclude_masks = [ exclude_mask ]
        img0.include_masks = [ include_mask ]

        img0.overlap_center = project_in ( point ( 0.0 , 0.0 ) )
        img1.overlap_center = point ( 0.0 , 0.0 )
        if ( maxx - minx ) < ( maxy - miny ) :
            img0.rotate_overlap_pano = True
        else :
            img0.rotate_overlap_pano = False

        # finally, we have to add img1's outline to img1 as it's include
        # mask - it doesn't have an exclude mask. Note it is only in this
        # special case where the symmetric case won't produce any masks,
        # since it does rely on it being done here.

        img1.include_masks = [ [ ( p.x , p.y )
                                 for p in outline ] ]
        img1.exclude_masks = []

        # now we're done with the 'mask with hole' and return True

        return True

    # the special case is dealt with, we continue with the ordinary:
    
    # later on we need the 'center' of the overlap. We make an
    # informed guess here by taking the mean of all crossover
    # points. Notice that the mean is calculated in real 3D
    # coordinates and reprojected onto the sphere.
    
    xm = 0.0
    ym = 0.0
    zm = 0.0
    for xp in crossings :
        xxp = transform_point ( xp , rtf1 )
        cart = polar2_to_cartesian3 ( xxp )
        xm += cart[0]
        ym += cart[1]
        zm += cart[2]
    xm /= len ( crossings )
    ym /= len ( crossings )
    zm /= len ( crossings )
    cart = normalize ( ( xm, ym, zm ) )
    pol = cartesian3_to_polar2 ( cart )
    center = point ( pol[0] , pol[1] )
    center = transform_point ( center , tf0 )
        
    outline = in_point_to_tip ( amended_outline )

    # now we generate separate lists for the parts of the 'other' outline
    # that lie inside this image. This is where we change to points
    # in 'this' image's coordinates.

    chains = []
    record = False
    for p in outline :
        if p.direction == 'in' :
            previous = None
            record = True
        if record :
            pin = p.twin
            pin.direction = p.direction
            if previous :
                previous.next = pin
            else :
                chains.append ( pin )
            previous = pin
        if p.direction == 'out' :
            record = False

    for xp in crossings :
        success = p0.take_in ( xp.twin )
        if not success :
            raise RunTimeError ( 'failed to insert crossover point %s' %
                                 xp.twin )

    outline = []
    for s in p0.segments :
        outline.append ( s.a )
                
    # of this contour, we use both parts - those that are
    # within the other image and those that aren't
    
    inchains = []
    outchains = []
    previous = outline[-1]
    for p in outline :
        # print ( 'outline:' , p )
        # note that when the OTHER image's contour comes IN
        # THIS image's contour goes OUT since we've traced
        # the contours both clockwise. Here we can append the
        # contour points without transformation. We make a doubly
        # linked list, since we need to go both ways
        if p.direction == 'in' :
            inchains.append ( p )
        elif p.direction == 'out' :
            outchains.append ( p )
        previous.right = p
        p.left = previous
        previous = p
            
    # finally, we have to connect the out nodes 
    # we walk the paths in different ways, depending on purpose.
    # first we generate the exclude mask. This is done by turning
    # 'left' at out nodes.
    
    for p in inchains :
        # print ( 'inchain:' , p )
        p.used = False

    exclude_masks = []

    for p in inchains :
        if p.used :
            continue
        start = p
        mask = []
        exclude_masks.append ( mask )
        while True :
            # print ( p )
            assert ( p.x , p.y ) not in mask
            mask.append ( ( p.x , p.y ) )
            if p.direction == 'out' :
                keep = 'left'
            elif p.direction == 'in' :
                p.used = True
                keep = 'next'
            if keep == 'left' :
                p = p.left
            else :
                p = p.next
            if p == start :
                break

    # next the include masks

    include_masks = []

    for p in inchains :
        p.used = False
    
    for p in inchains :
        if p.used :
            continue
        start = p
        mask = []
        include_masks.append ( mask )

        while True :
            mask.append ( ( p.x , p.y ) )
            if p.direction == 'out' :
                keep = 'right'
            elif p.direction == 'in' :
                p.used = True
                keep = 'next'
            if keep == 'right' :
                p = p.right
            else :
                p = p.next
            if p == start :
                break

    # we use the include masks to find the maximal extents
    
    minx =  1000000.0
    maxx = -1000000.0
    miny =  1000000.0
    maxy = -1000000.0
    
    for m in include_masks :
        for p in m :
            minx = min ( p[0] , minx )
            miny = min ( p[1] , miny )
            maxx = min ( p[0] , maxx )
            maxy = min ( p[1] , maxy )

    dx = maxx - minx
    dy = maxy - miny

    if dx < dy :
        rotate_pano = True
    else :
        rotate_pano = False

    # finally we attach all the mask data to 'this' image as additional
    # attributes to conveniently pass them around instead of having to
    # pass around numerous return values.

    img0.exclude_masks = exclude_masks
    img0.include_masks = include_masks
    img0.overlap_center = center
    img0.rotate_overlap_pano = rotate_pano

    # we return True to signal that masks have been created
    return True

# attach_exclude_masks will aply the previously calculated masks to exclude
# the non-overlapping parts from the image in question.

def attach_exclude_masks ( img ) :

    # the mask data are calculated with the origin in the center of the
    # image, we have to change that to corner-relative pixel coordinates.
    # also, due to a bug in hugin masking code, we have to shift mask
    # points on the left image margin by one pixel to the left.
    
    wh = img.getWidth() / 2.0
    hh = img.getHeight() / 2.0
    
    for m in img.exclude_masks :
        Mask = hsi.MaskPolygon()
        for p in m :
            cpx = p[0] + wh
            cpy = p[1] + hh
            if abs ( cpx ) < POSITION_DELTA :
                cpx = mask_offset
            Mask.addPoint ( hsi.FDiff2D ( cpx , cpy ) )
        img.addMask ( Mask )

# to determine if it's worth our while to consider the overlap between
# two images for CP detection, we need the size of the overlapping area.
# This can be easily computed from the 'include' masks:

def calculate_overlap_ratio ( img ) :

    area2 = 0.0
    for m in img.include_masks :
        previous = m[-1]
        for current in m :
            dy = previous[1] - current[1]
            xsum = current[0] + previous[0]
            area2 += dy * xsum                
            previous = current

    overlap_area = area2 / 2.0
    vpr ( 'overlap area:' , overlap_area , 'pixels' )
    w = img.getWidth()
    h = img.getHeight()
    img.overlap_ratio = overlap_area / ( w * h )
    vpr ( 'overlap ratio:' , img.overlap_ratio )

# the next routine will calculate the overlap of the images a and b
# in the panorama pano and generate control points for the overlapping
# area using the warped overlap method.
# overlap_threshold defines from what size overlap the images will be
# processed at all. The value refers to the normalized overlap area,
# that's the ration ove pixels in overlapping areas to total pixels.
# scaling_factor defines by how much the images are scaled compared to
# the 'optimal size' that hugin would calculate for the panorama that
# generates the warped images.

def cps_from_overlap ( pano , a , b ) :

    # hugin doesn't like wrongly-ordered numbers...

    if a > b :
        help = a
        a = b
        b = help
    
    # we make a subset panorama with only these two images and
    # set it to 360 degrees X 180 degrees equirect with width = 360
    # and height = 180, so that we can use panorama coordinates
    # directly as lat/lon values

    subset = hsi.UIntSet ( [ a , b ] )
    subpano = pano.getSubset ( subset )

    # if the images aren't active, nona will not process them.

    subpano.activateImage ( 0 )
    subpano.activateImage ( 1 )
    
    # what were image a and b in the original pano are now 0 and 1

    img0 = subpano.getImage ( 0 )
    img0.exclude_masks = []
    img0.include_masks = []
    img0.overlap_center = None
    img0.rotate_overlap_pano = False

    img1 = subpano.getImage ( 1 )
    img1.exclude_masks = []
    img1.include_masks = []
    img1.overlap_center = None
    img1.rotate_overlap_pano = False

    pano_options = subpano.getOptions()
    pano_options.setProjection ( hsi.PanoramaOptions.EQUIRECTANGULAR )
    pano_options.setHFOV ( 360.0 )
    pano_options.setWidth ( 360 )
    pano_options.setHeight ( 180 )

    subpano.setOptions ( pano_options )

    # now we call the routine to set exclude masks for the parts
    # of the images which don't overlap. Notice that this process
    # works only on the y, p, and r values found in the pto
    # we're working on, it may be totally wrong if these values
    # aren't roughly correct. With x degrees hov and 1/3 overlap,
    # an error of, say, x/20 degrees won't make to much difference.
    
    mask_nonoverlaps ( subpano , img0 , img1 )
    calculate_overlap_ratio ( img0 )
    mask_nonoverlaps ( subpano , img1 , img0 )
    calculate_overlap_ratio ( img1 )

    if (     img0.overlap_ratio <= args.threshold
         and img1.overlap_ratio <= args.threshold ) :
        vpr ( 'overlap is below or equal specified threshold' )
        return 0

    if (     img0.overlap_ratio > args.ceiling
         and img1.overlap_ratio > args.ceiling ) :
        vpr ( 'overlap is above specified ceiling' )
        return 0

    attach_exclude_masks ( img0 )
    attach_exclude_masks ( img1 )

    # if we're here, the images have qualified for being submitted to the
    # warped overlap method.
    # just for the record we write the panorama with the applied exclude
    # masks to disk
    
##    ofs = hsi.ofstream ( 'subset0.pto' )
##    subpano.writeData ( ofs )
##    del ofs

    # we transform the panorama so that:
    # - the assumed center of the overlap is in the center of the pano
    # - the larger extent of the overlap is parallel to the pano's horizon
    # these transformations assure that the distortions of the warped
    # images we create from the overlap are minimal.

    # we need the reverse transform, from image to pano, for img0,
    # so we can shift the center:
    
    rtf0 = hsi.Transform()
    rtf0.createInvTransform ( img0 , pano_options )

    # we transform our center point to pano coordinates, which amount
    # to lon/lat since we adjusted the panorama dimensions to 360X180
    
    center = transform_point ( img0.overlap_center , rtf0 )

    # now we can use center's coordinates directly to specify the
    # needed rotations to put center at the pano's origin:
    
    rp = hsi.RotatePanorama ( subpano , -center.x , 0.0 , 0.0 )
    rp.runAlgorithm()
    rp = hsi.RotatePanorama ( subpano , 0.0 , center.y , 0.0 )
    rp.runAlgorithm()

    # if the masking routine has determined that we'd be better off
    # with the pano rotated 90 degrees, we do that as well
    
    if img0.rotate_overlap_pano :
        rp = hsi.RotatePanorama ( subpano , 0.0 , 0.0 , 90.0 )
        rp.runAlgorithm()

    # we get the transforms from panospace to image coordinates
    # for this panorama, since later on our control points from
    # the warped imaged will have to be mapped back to the original
    # images with this very transform. We also keep a record of
    # the images' dimensions, expressed as an offset, since later
    # on we have to convert from image coordinates, which are
    # with origin in the image center, to pixel coordinates used
    # by the control points which put the origin in a corner.
    
    pano_options = subpano.getOptions()
    img0 = subpano.getImage ( 0 )
    img1 = subpano.getImage ( 1 )

    w0 = img0.getWidth()
    h0 = img0.getHeight()
    xcenter0 = w0 / 2.0
    ycenter0 = h0 / 2.0
    
    w1 = img1.getWidth()
    h1 = img1.getHeight()
    xcenter1 = w1 / 2.0
    ycenter1 = h1 / 2.0

    tf0 = hsi.Transform()
    tf0.createTransform ( img0 , pano_options )

    tf1 = hsi.Transform()
    tf1.createTransform ( img1 , pano_options )

    # subpano is the panorama we pass to nona for generation of
    # the warped overlap images. We need to increase the size to
    # near-original (like the 'optimal size' button in hugin)
    # here we may choose a different scale to further influence
    # the outcome of the CPG, depending on purpose:
    # to get as many CPs as possible, like for calibration, using
    # the original size or even a larger size should be optimal,
    # whereas if it's only a matter of gaining well-distributed CPs,
    # the size can be shrunk.

    scale_calc = hsi.CalculateOptimalScale ( subpano )
    scale_calc.run()
    optimal_width = scale_calc.getResultOptimalWidth()

    # we make sure we use an even number for the width - and it has to
    # be integral as well:

    subpano_width = 2 * int ( optimal_width * ( args.scale / 2.0 ) ) 

    vpr ( 'setting subpano width to' , subpano_width )
    
    # we set the width of the panorama
    
    pano_options.setWidth ( subpano_width )
    subpano.setOptions ( pano_options )

    # and write it to disk so we can pass it to nona
    
    if args.prolific :
        warped_image_basename = '%s_%03d_%03d' % ( args.basename , a , b  )
        nona_file_name = warped_image_basename + '.pto'
        warped_pto = warped_image_basename + '_warped.pto'
    else :
        warped_image_basename = args.basename
        nona_file_name = '%s_base.pto' % args.basename
        warped_pto = '%s_warped.pto' % args.basename

    ofs = hsi.ofstream ( nona_file_name )
    subpano.writeData ( ofs )
    del ofs
    
    # we let nona execute this panorama to yield the warped overlaps.
    # the warped images are saved under the name specified
    # by warped_image_basename.
    # Notice that we assume nona is in the system path.

    vpr ( 'creating warped overlap images with' , nona_file_name )
    
    command = [ 'nona' ,
                '-o' ,
                warped_image_basename ,
                nona_file_name ]

    retval = subprocess.call ( command )

    # we check if nona really made the two images to process
    # and only proceed then - there may be corner cases when an
    # overlap was detected for (a,b) but not (b,a) - in this case
    # only one image might be made by nona and assuming there are two
    # (with the intended names) will trip the CPG which won't find
    # the second image

    nona_output = [ '%s0000.tif' % warped_image_basename ,
                    '%s0001.tif' % warped_image_basename ]

    for f in nona_output :
        if not os.path.exists ( f ) :
            return 0
        
    # next generate CPs from the warped images. What I do currently is
    # use autopano-sift-c with hardcoded parameters. apsc isn't easy
    # to handle for the purpose, so it has to be tricked into doing
    # the job. First, even though the images are really 360X180 equirect
    # in cropped TIFF format, as made by nona, apsc doesn't expect
    # such input and fails spectacularly if we pass 360 hfov and
    # RECTILINEAR as input. I think what happens is that it makes funny
    # assumptions and transforms the images somehow, resulting in
    # zero CPs.
    # so we trick autopano-sift-c to take the images, which are really
    # 360x180 equirect, in cropped tiff format, as 30 degree hfov
    # rectilinear so that it doesn't warp them itself.

    # TODO: extend to use more than just the hardcoded one

    if args.cpg == 'cpfind' :

        ofs = hsi.ofstream ( '_%s' % warped_pto )
        warp = hsi.Panorama()
        wi0 = hsi.SrcPanoImage ( nona_output[0] )
        wi1 = hsi.SrcPanoImage ( nona_output[1] )
        warp.addImage ( wi0 )
        warp.addImage ( wi1 )
        warp.writeData ( ofs )
        del ofs
            
        command = [ 'cpfind' ,
                    '--fullscale' ,
                    '-o' , warped_pto ,
                    '_%s' % warped_pto ]

    elif args.cpg == 'apsc' :
        
        # apsc works just fine like this:

        vpr ( 'making pto for CP detection: %s' % warped_pto )

        command = [ 'autopano-sift-c' ,
                    '--projection' , '%d,30' % hsi.PanoramaOptions.RECTILINEAR ,
                    '--maxdim' , '%d' % subpano_width ,
                    '--maxmatches' , '0' ,
                    warped_pto ] + nona_output

    else :

        raise RunTimeError ( 'only cpfind and apsc are allowed as CPG' )

    vpr ( 'cp detection:' , command )    
    retval = subprocess.call ( command )

    # now the CPG has made a panorama for us, and the only thing we
    # care for from that is the CPs it's generated. Let's get them:
    
    ifs = hsi.ifstream ( warped_pto )
    warp = hsi.Panorama()
    warp.readData ( ifs )
    del ifs

    # we take the lot as a CPVector:
    
    cpv = warp.getCtrlPoints()
    vpr ( 'CPG found %d CPs' % len ( cpv ) )

    # We want to put the remapped CPs into this CPVector:
    
    tcpv = hsi.CPVector()

    # if we're prolific, we delete the CPs present in subpano
    # so only newly generated points will be seen
    
    if args.prolific:
        subpano.setCtrlPoints ( tcpv )
        
    # the images have been saved by nona as cropped tiff. We need
    # to know the absolute positions of the CPs, so we have to
    # get the offsets. I do this using tiffdump, maybe there's
    # a better way?
    # Anyway, cropped TIFF is a fine thing, it saves processing
    # time and disk space, so we accept the little extra work:
    
    xoff1 , yoff1 = get_tiff_offset ( nona_output[0] )
    xoff2 , yoff2 = get_tiff_offset ( nona_output[1] )

    # we need to get back from absolute panorama coordinates to
    # 360X180 degrees, so we can map the CPs back to original image
    # coordinates.
    
    scale = 360.0 / float ( subpano_width )
    cps_added = 0
    
    for cp in cpv :
        # the coordinates in cpv refer to the warped images. Since
        # these are, really, 360x180 equirect in cropped TIFF, all we
        # need to do with the coordinates the CPG has generated is
        # - add the TIFF crop offsets
        # - scale to 360X180
        # - shift the origin to the center
        # vpr ( 'cp: ' , cp.x1 , cp.y1 , cp.x2 , cp.y2 )
        x1 = scale * ( cp.x1 + xoff1 ) - 180.0
        x2 = scale * ( cp.x2 + xoff2 ) - 180.0
        y1 = scale * ( cp.y1 + yoff1 ) - 90.0
        y2 = scale * ( cp.y2 + yoff2 ) - 90.0
        # vpr ( 'angles' , x1 , y1 , x2 , y2 )
        # - transform these angles into original image coordinates
        cp0 = transform_point ( point ( x1 , y1 ) , tf0 )
        cp1 = transform_point ( point ( x2 , y2 ) , tf1 )
        # vpr ( 'in img0:' , cp0 , 'in img1' , cp1 )
        # - convert from image-center origin image coordinates
        #   to corner origin pixel coordinates
        x1 = cp0.x + xcenter0
        y1 = cp0.y + ycenter0
        x2 = cp1.x + xcenter1
        y2 = cp1.y + ycenter1
        # - generate a ControlPoint object if the point is inside
        #   the image - sometimes the CPGs produce CPs outside the
        #   overlap area, I haven't found out why.
        if (    x1 < 0.0 or x1 > w0
             or y1 < 0.0 or y1 > h0
             or x2 < 0.0 or x2 > w1
             or y2 < 0.0 or y2 > h1 ) :
            continue
        tcp = hsi.ControlPoint ( a , x1 , y1 , b , x2 , y2 ,
                                 hsi.ControlPoint.X_Y )
        cps_added += 1
        # vpr ( 'as CP:' , tcp.x1 , tcp.y1 , tcp.x2 , tcp.y2 )
        # - and put that into the panorama
        # tcpv.append ( tcp )
        pano.addCtrlPoint ( tcp )
        if args.prolific:
            subpano.addCtrlPoint ( tcp )

    # if we're prolific, we write the nona file with added CPs
    # so the yield of the CPG can be inspected
    
    if args.prolific :
        ofs = hsi.ofstream ( nona_file_name )
        subpano.writeData ( ofs )
        del ofs
        
    # finally we return the number of control points we've added
    # to the panorama

    return cps_added

# from now on it's basically administration: determine which image pairs
# are looked at and what the parameters are.

def process_image_set ( pano , image_set ) :
    
    vpr ( 'processing image set' , image_set )

    # the user may have specified a 'focus' image, in which case
    # only combinations of the focus image with the other images
    # are looked at. Otherwise, all combinations

    if args.focus :
        pairs = [ ( args.focus , image )
                  for image in image_set
                  if args.focus != image ]
    else :
        pairs = list ( itertools.combinations ( image_set , 2 ) )

    # finally we use the workhorse routine on all pairs:

    total_cps_added = 0
    
    for pair in pairs :
        vpr ( 'examining image pair' , pair )
        cps_added = cps_from_overlap ( pano , *pair )
        total_cps_added += cps_added
        vpr ( 'pair' , pair , 'CPs added' , cps_added )

    return total_cps_added

# entry() is what is called directly when the hugin plugin interface calls
# this script. Since there is no parameter passing yet, the routine figures
# out which image pairs to process by looking at which images are 'active'.

def entry ( pano ) :

    # we emulate a parameter set as it would have been produced
    # by the argument parser in the CLI version:
    
    global args
    args = argobj()

    vpr ( 'entry: parameters used for this run:' )
    vpr ( 'ceiling:' , args.ceiling )
    vpr ( 'focus:  ' , args.focus )
    vpr ( 'margin: ' , args.margin )
    vpr ( 'scale:  ' , args.scale )
    vpr ( 'thresh: ' , args.threshold )
    vpr ( 'prolific' , args.prolific )
    vpr ( 'verbose:' , args.verbose )

    # we restrict the search to the active images
    # in the panorama. This is working around the fact that
    # as of this writing plugins can't receive parameters,
    # so we use the 'active' state of the images for selection.

    image_set = [ image for image in pano.getActiveImages() ]

    # we doublecheck: if only one image is active, we change
    # our logic and take the only active image as our focus
    # and all images as image set:

    if len ( image_set ) == 1 :
        args.focus = image_set[0]
        image_set = [ int ( image )
                      for image
                      in range ( pano.getNrOfImages() ) ]

    total_cps_added = process_image_set ( pano , image_set )
    return total_cps_added

# main() is called if the program is called from the command line.
# In this case we have a proper set of arguments to process:

def main() :

    # when called from the command line, we import the argparse module
    
    import argparse

    # and create an argument parser
    
    parser = argparse.ArgumentParser (
        formatter_class=argparse.RawDescriptionHelpFormatter ,
        description = gpl + '''
    woa will look at every possible pair of images that can be made
    from the image numbers passed as -i parameters, or from all images
    in the pto if no -i parameters are passed. If the images in a pair
    overlap significantly, the overlapping parts will be warped and
    a CPG run on them. The resulting CPs will be transferred back into
    the original pto file.
    ''' )

    parser.add_argument('-c', '--ceiling',
                        metavar='<overlap ceiling>',
                        default = 1.0 ,
                        type=float,
                        help='ignore overlaps above this value (0.0-1.0)')

    parser.add_argument('-g', '--cpg',
                        metavar='<cpfind or apsc>',
                        default = 'apsc',
                        type=str,
                        help='choose which CP generator to use')

    parser.add_argument('-f' , '--focus' ,
                        metavar='<image number>',
                        default=None,
                        type=int,
                        help='only look for overlaps with this image')

    parser.add_argument('-i', '--images',
                        metavar='<image number>',
                        default = [] ,
                        nargs = '+',
                        type=int,
                        help='image numbers to process')

    parser.add_argument('-m', '--margin',
                        metavar='<margin>',
                        default = 0 ,
                        type=int,
                        help='widen examined area [in pixels]')

    parser.add_argument('-o', '--output',
                        metavar='<output file>',
                        default = None,
                        type=str,
                        help='write output to a different file')

    parser.add_argument('-b', '--basename',
                        metavar='<base name>',
                        default = 'woa',
                        type=str,
                        help='common prefix for intermediate files')

    parser.add_argument('-p', '--prolific',
                        action='store_true',
                        help='keep all intermediate files')

    parser.add_argument('-t', '--threshold',
                        metavar='<overlap threshold>',
                        default = 0.0 ,
                        type=float,
                        help='ignore overlaps below this threshold (0.0-1.0)')

    parser.add_argument('-s', '--scale',
                        metavar='<scaling factor>',
                        default = 0.25 ,
                        type=float,
                        help='scaling for warped overlap images')

    parser.add_argument('-v' , '--verbose',
                        action='store_true',
                        help='produce verbose output')

    parser.add_argument('input' ,
                        metavar = '<pto file>' ,
                        type = str ,
                        help = 'pto file to be processed' )

# we may add logging at some point...

##    parser.add_argument('-l', '--log',
##                        metavar='<log file>',
##                        type=argparse.FileType('w'),
##                        help='write log file')

    # if the argument count is less than two, there aren't any arguments.
    # Currently this is considered an error and triggers help output:

    if len ( sys.argv ) < 2 :
        parser.print_help()
        return

    # we parse the arguments into a global variable so we can access
    # them from everywhere without having to pass them around
    
    global args
    
    args = parser.parse_args()

    vpr ( 'main: parameters used for this run:' )
    vpr ( 'ceiling:' , args.ceiling )
    vpr ( 'focus:  ' , args.focus )
    vpr ( 'images: ' , args.images )
    vpr ( 'margin: ' , args.margin )
    vpr ( 'output: ' , args.output )
    vpr ( 'scale:  ' , args.scale )
    vpr ( 'thresh: ' , args.threshold )
    vpr ( 'prolific' , args.prolific )
    vpr ( 'ptofile:' , args.input )
    vpr ( 'verbose:' , args.verbose )

    # first we see if we can open the input file

    ifs = hsi.ifstream ( args.input )
    if not ifs.good() :
        raise RunTimeError ( 'cannot open input file %s' % args.input )

    pano = hsi.Panorama()
    success = pano.readData ( ifs )
    del ifs
    if success != hsi.DocumentData.SUCCESSFUL :
        raise RunTimeError ( 'input file %s contains invalid data' % args.input )

    # let's see if the image numbers passed are correct if there are any
    
    ni = pano.getNrOfImages()
    vpr ( 'found %d images in panorama' % ni )
    if ni < 2 :
        raise RunTimeError ( 'input file %s contains less than two images'
                             % args.input )

    for nr in range ( ni ) :
        img = pano.getImage ( nr )
        name = img.getFilename()
        # check if we can access this image
        # TODO: doublecheck in folder of pto file
        if not os.path.exists ( name ) :
            raise RunTimeError ( "cannot access image %s (try cd to pto's directory)" % name )
        vpr ( 'image %d: %s' % ( nr , name ) )
        
    for nr in args.images :
        if nr < 0 or nr >= ni :
            raise RunTimeError ( 'no image number %d in input file' % nr )

    if args.focus :
        if not ( 0 <= args.focus < ni ) :
            raise RunTimeError ( 'invalid focus image %d' % args.focus )
            
    if args.threshold > args.ceiling :
        raise RunTimeError ( 'threshold must be below ceiling' )
            
    # if a separate output file was chosen, we open it now to avoid
    # later disappointments
    
    if args.output:
        ofs = hsi.ofstream ( args.output )
        if not ofs.good() :
            raise RunTimeError ( 'cannot open output file %s' % args.output )

    if len ( args.images ) :

        # if specific image numbers have been passed as arguments,
        # we look for CPs no matter if they're active or not

        image_set = [ int ( image ) for image in args.images ]

    else :

        # otherwise, we look at all images. Note that this behaviour
        # is different to the plugin version which looks at the 'active'
        # state of the images.
        
        image_set = [ int ( image )
                      for image
                      in range ( pano.getNrOfImages() ) ]

    # all set up. We process the image set

    total_cps_added = process_image_set ( pano , image_set )

    # if no different output file was specified, overwrite the input
    
    if not args.output :
        ofs = hsi.ofstream ( args.input )
        if not ofs.good() :
            raise RunTimeError ( 'cannot open file %s for output' % args.input )

    success = pano.writeData ( ofs )
    del ofs
    if success != hsi.DocumentData.SUCCESSFUL :
        raise RunTimeError ( 'error writing pano to %s' % args.input )
    
    # done.
    
    return total_cps_added

# finally the test for the invocation mode:
# are we main? This means we've been called from the command line
# as a standalone program, so we have to go through the main() function.

if __name__ == "__main__":

    try:

        total_cps_added = main()

    except RunTimeError as e :

        print ( 'Run Time Error: %s' % e )
        sys.exit ( -1 )

    sys.exit ( total_cps_added )
