/**
 * @file AssistantMakefilelibExport.cpp
 * @brief
 *  
 * @author Thomas Modes
 */

/*  This is free software; you can redistribute it and/or
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

#include "AssistantMakefilelibExport.h"

#include <makefilelib/char_type.h>
#include <iostream>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <locale>
#include <makefilelib/Comment.h>
#include <makefilelib/Variable.h>
#include <makefilelib/VariableDef.h>
#include <makefilelib/VariableRef.h>
#include <makefilelib/MakefileItem.h>
#include <makefilelib/Makefile.h>
#include <makefilelib/AutoVariable.h>
#include <makefilelib/Newline.h>
#include <makefilelib/Rule.h>
#include <makefilelib/Conditional.h>
#include <makefilelib/Manager.h>
#include <makefilelib/Anything.h>

#include <panodata/PanoramaData.h>
#include <hugin_utils/utils.h>
#include "algorithms/optimizer/ImageGraph.h"

namespace HuginBase
{
using namespace makefile;
using namespace std;
using namespace vigra;
namespace mf = makefile;

/// Automates an very often occuring sequence
#define  newVarDef(var, name, ...) \
mf::Variable* var = mgr.own(new mf::Variable(name, __VA_ARGS__)); \
var->getDef().add();

bool AssistantMakefilelibExport::createItems()
{
    // we use this Variable for initializing pointers that get an object only under certain conditions
    mf::Variable* nullvar = mgr.own(new mf::Variable("NOT_DEFINED", "This_variable_has_not_been_defined"));

    mgr.own_add((new Comment(
        "makefile for automatic panorama generating, created by hugin using the new makefilelib")));

#ifdef _WINDOWS
    mgr.own_add(new Comment("Force using cmd.exe"));
    mf::Variable* winshell = mgr.own(new mf::Variable("SHELL", getenv("ComSpec")));
    winshell->getDef().add();
#endif

    //----------
    // set the tool commands
    mgr.own_add(new Comment("Tool configuration"));
    newVarDef(vicpfind, "ICPFIND", progs.icpfind)
    newVarDef(vceleste, "CELESTE", progs.celeste)
    newVarDef(vcheckpto, "CHECKPTO", progs.checkpto)
    newVarDef(vcpclean, "CPCLEAN", progs.cpclean)
    newVarDef(vautooptimiser, "AUTOOPTIMISER", progs.autooptimiser)
    newVarDef(vpanomodify, "PANO_MODIFY", progs.pano_modify)

    newVarDef(vproject, "PROJECT", projectFile, Makefile::MAKE);
    newVarDef(vprojectShell, "PROJECT_SHELL", projectFile,Makefile::SHELL)

    Rule* all = mgr.own(new Rule());
    all->addTarget("all");
    all->addPrereq(vproject->getRef());

    mf::string outinproject=cstr(" -o ")+vprojectShell->getRef()+cstr(" ")+vprojectShell->getRef();
    bool runicp=(pano.getNrOfCtrlPoints()==0);
    if(!runicp)
    {
        //we check, if all images are connected
        //if not, we run also icpfind
        CPGraph graph;
        createCPGraph(pano, graph);
        CPComponents comps;
        runicp=findCPComponents(graph, comps)>1;
    };
    //build commandline for icpfind
    if(runicp)
    {
        //create cp find
        all->addCommand(vicpfind->getRef()+outinproject);
        //building celeste command
        if(runCeleste)
        {
            mf::string celesteCommand=vceleste->getRef()+" ";
            valuestream.str("");    // clear the streams buffer
            valuestream << " -f " << celesteThreshold;
            celesteCommand+=valuestream.str()+" ";
            if(celesteSmallRadius)
                celesteCommand+="-r 1";
            all->addCommand(celesteCommand+outinproject);
        };
        //building cpclean command
        if(runCPClean)
        {
            all->addCommand(vcheckpto->getRef()+" "+vprojectShell->getRef());
            all->addCommand(vcpclean->getRef()+outinproject);
        };
    };
    //now optimise all
    all->addCommand(vcheckpto->getRef()+" "+vprojectShell->getRef());
    all->addCommand(vautooptimiser->getRef()+" -a -m -l -s"+outinproject);
    // if necessary scale down final pano
    valuestream.str("");
    if(scale<1)
    {
        valuestream << " --canvas=" << hugin_utils::roundi(scale*100) << "%%";
    };
    all->addCommand(vpanomodify->getRef()+valuestream.str()+cstr(" --crop=AUTO")+outinproject);
    all->add();
    return true;
}

}; //namespace

