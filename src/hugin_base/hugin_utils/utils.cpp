// -*- c-basic-offset: 4 -*-

/** @file utils.cpp
 *
 *  @author Pablo d'Angelo <pablo.dangelo@web.de>
 *
 *  $Id$
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include "utils.h"

#ifdef WIN32
    #include <sys/utime.h>
#else
    #include <sys/time.h>
#endif
#include <time.h>
#include <stdio.h>
#include <cstdio>


namespace hugin_utils {
    
#ifdef UNIX_LIKE
std::string CurrentTime()
{
  char tmp[100];
  struct tm t;
  struct timeval tv;
  gettimeofday(&tv,NULL);
  localtime_r((time_t*)&tv.tv_sec, &t); // is the casting safe?
  strftime(tmp,99,"%H:%M:%S",&t);
  sprintf(tmp+8,".%06ld", (long)tv.tv_usec);
  return tmp;
}
#else
std::string CurrentTime()
{
    // FIXME implement for Win
    return "";
}
#endif


std::string getExtension(const std::string & basename2)
{
	std::string::size_type idx = basename2.rfind('.');
    // check if the dot is not followed by a \ or a /
    // to avoid cutting pathes.
    if (idx == std::string::npos) {
        // no dot found
		return std::string("");
    }
#ifdef UNIX_LIKE
    // check for slashes after dot
    std::string::size_type slashidx = basename2.find('/', idx);
    if ( slashidx == std::string::npos)
    {
        return basename2.substr(idx+1);
    } else {
        return std::string("");
    }
#else
    // check for slashes after dot
    std::string::size_type slashidx = basename2.find('/', idx);
    std::string::size_type backslashidx = basename2.find('\\', idx);
    if ( slashidx == std::string::npos &&  backslashidx == std::string::npos)
    {
        return basename2.substr(idx+1);
    } else {
		return std::string("");
    }
#endif
}

std::string stripExtension(const std::string & basename2)
{
    std::string::size_type idx = basename2.rfind('.');
    // check if the dot is not followed by a \ or a /
    // to avoid cutting pathes.
    if (idx == std::string::npos) {
        // no dot found
        return basename2;
    }
#ifdef UNIX_LIKE
    std::string::size_type slashidx = basename2.find('/', idx);
    if ( slashidx == std::string::npos)
    {
        return basename2.substr(0, idx);
    } else {
        return basename2;
    }
#else
    // check for slashes after dot
    std::string::size_type slashidx = basename2.find('/', idx);
    std::string::size_type backslashidx = basename2.find('\\', idx);
    if ( slashidx == std::string::npos &&  backslashidx == std::string::npos)
    {
        return basename2.substr(0, idx);
    } else {
        return basename2;
    }
#endif
}

std::string stripPath(const std::string & filename)
{
#ifdef UNIX_LIKE
    std::string::size_type idx = filename.rfind('/');
#else
    std::string::size_type idx1 = filename.rfind('\\');
    std::string::size_type idx2 = filename.rfind('/');
    std::string::size_type idx;
    if (idx1 == std::string::npos) {
        idx = idx2;
    } else if (idx2 == std::string::npos) {
        idx = idx1;
    } else {
        idx = std::max(idx1, idx2);
    }
#endif
    if (idx != std::string::npos) {
//        DEBUG_DEBUG("returning substring: " << filename.substr(idx + 1));
        return filename.substr(idx + 1);
    } else {
        return filename;
    }
}

std::string getFolder(const std::string & filename)
{
#ifdef UNIX_LIKE
    std::string::size_type idx = filename.rfind('/');
    std::string f = filename.substr(0, idx);
    std::string::size_type idy = f.rfind('/');
#else

    std::string::size_type idx1 = filename.rfind('\\');
    std::string::size_type idx2 = filename.rfind('/');
    std::string::size_type idx;
    std::string::size_type idy;
    std::string f;
    if (idx1 == std::string::npos) {
        idx = idx2;
        f = filename.substr(0, idx);
        idy = f.rfind('/', idx);
    } else if (idx2 == std::string::npos) {
        idx = idx1;
        f = filename.substr(0, idx);
        idy = f.rfind('\\', idx);
    } else {
        idx = std::max(idx1, idx2);
        f = filename.substr(0, idx);
        if (idx == idx2) {
            idy = f.rfind('/', idx);
        } else {
            idy = f.rfind('\\', idx);
        }
    }
#endif
    if (idx != std::string::npos) {
//        DEBUG_DEBUG("returning substring: " << f.substr(idy, idx));
        return f.substr(idy+1, idx-idy);
    } else {
        return filename;
    }
}



std::string getPathPrefix(const std::string & filename)
{
#ifdef UNIX_LIKE
    std::string::size_type idx = filename.rfind('/');
#else
    std::string::size_type idx1 = filename.rfind('\\');
    std::string::size_type idx2 = filename.rfind('/');
    std::string::size_type idx;
    if (idx1 == std::string::npos) {
        idx = idx2;
    } else if (idx2 == std::string::npos) {
        idx = idx1;
    } else {
        idx = std::max(idx1, idx2);
    }
#endif
    if (idx != std::string::npos) {
//        DEBUG_DEBUG("returning substring: " << filename.substr(idx + 1));
        return filename.substr(0, idx+1);
    } else {
        return "";
    }
}

std::string doubleToString(double d, int digits)
{
    char fmt[10];
    if (digits < 0) {
        strcpy(fmt,"%f");
    } else {
        std::sprintf(fmt,"%%.%df",digits);
    }
    char c[1024];
    c[1023] = 0;
#ifdef _MSC_VER
    _snprintf (c, 1023, fmt, d);
#else
    snprintf (c, 1023, fmt, d);
#endif

    std::string number (c);

    int l = (int)number.length()-1;

    while ( l != 0 && number[l] == '0' ) {
      number.erase (l);
      l--;
    }
    if ( number[l] == ',' ) {
      number.erase (l);
      l--;
    }
    if ( number[l] == '.' ) {
      number.erase (l);
      l--;
    }
    return number;
}

    void ControlPointErrorColour(const double cperr, 
        double &r,double &g, double &b)
    {
        //Colour change points
        double xp1=5;
        double xp2=10;

        if ( cperr<= xp1) 
        {
            //low error
            r = cperr / xp1;
            g = 0.75;
        }
        else
        {
            r = 1.0;
            g = 0.75 * ( (1.0-std::min<double>(cperr-xp1,(xp2-xp1))/(xp2-xp1)));
        } 
        b = 0.0;
    }

    void TranslateText(){
        // locate the translations
    #if _WINDOWS
        char buffer[MAX_PATH];//always use MAX_PATH for filepaths
        GetModuleFileName(NULL,buffer,sizeof(buffer));
        std::string working_path=(buffer);
        std::string locale_path="";
        //remove filename
        std::string::size_type pos=working_path.rfind("\\");
        if(pos!=std::string::npos)
        {
            working_path.erase(pos);
            //remove last dir: should be bin
            pos=working_path.rfind("\\");
            if(pos!=std::string::npos)
            {
                working_path.erase(pos);
                //append path delimiter and path
                working_path.append("\\share\\hugin\\data\\");
                locale_path=working_path;
            }
        }
    #elif defined MAC_SELF_CONTAINED_BUNDLE
        char path[PATH_MAX + 1];
        uint32_t size = sizeof(path);
        std::string locale_path("");
        if (_NSGetExecutablePath(path, &size) == 0)
        {
          locale_path=dirname(path);
          locale_path.append("/../Resources/xrc/");
        }
    #else
          std::string locale_path = (INSTALL_DATA_DIR);
    #endif
        // and tell gettext where the translations are and which one to use
        bindtextdomain( "hugin", locale_path.c_str() );
        textdomain( "hugin" );
    }
    
} //namespace
