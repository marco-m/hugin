// -*- c-basic-offset: 4 -*-
/** @file CalculateOptimalROI.cpp
 *
 *  @author <cnidarian>
 *
 *  $Id: CalculateOptimalROI.cpp 2510 2009-9-9 cnidarian $
 *
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

#include "CalculateOptimalROI.h"

namespace HuginBase {

using namespace hugin_utils;

//uses the area as that major searching factor
//else uses width+heigh like the perimeter
#define USEAREA

///
bool CalculateOptimalROI::calcOptimalROI(PanoramaData& panorama)
{
    activeImages=panorama.getActiveImages();
    if (activeImages.size() == 0)
        return false;

    printf("Down to Algorithm\n");
    PanoramaOptions opt = panorama.getOptions();
    
    o_optimalSize=opt.getSize();
    if(o_optimalSize.x==0 || o_optimalSize.y==0)
        return false;
    o_optimalROI=vigra::Rect2D(0,0,o_optimalSize.x,o_optimalSize.y);
    try
    {
        testedPixels.resize(o_optimalSize.x*o_optimalSize.y,false);
        pixels.resize(o_optimalSize.x*o_optimalSize.y,false);
    }
    catch(std::bad_alloc&)
    {
        //could not allocate enough memory
        return false;
    };

    for (UIntSet::const_iterator it=activeImages.begin(); it!=activeImages.end(); it++)
    {
        const SrcPanoImage &img=panorama.getImage(*it);
        PTools::Transform *transf=new PTools::Transform();
        transf->createTransform(img,opt);
        transfMap.insert(std::pair<unsigned int,PTools::Transform*>(*it,transf));
    }
    
    printf("Calculate the cropping region\n");
    autocrop();
    
    o_optimalROI=vigra::Rect2D(best.left,best.top,best.right,best.bottom);
    printf("Crop %dx%d - %dx%d\n",o_optimalROI.left(),o_optimalROI.top(),o_optimalROI.right(),o_optimalROI.bottom());
    printf("Crop Size %dx%d\n",o_optimalROI.width(),o_optimalROI.height());
    
//debug image of the region calculated
#if 0
    FILE *outstream=fopen("/tmp/test.pgm","wb");
    fprintf(outstream,"P5\n");
    fprintf(outstream,"%d %d\n",o_optimalSize.x,o_optimalSize.y);
    fprintf(outstream,"255\n");
    fwrite(tmp,o_optimalSize.x*o_optimalSize.y,1,outstream);
    fclose(outstream);
#endif

    //clean up on demand
    for(std::map<unsigned int,PTools::Transform*>::iterator it=transfMap.begin();it!=transfMap.end();it++)
    {
        delete (*it).second;
    };

    return true;
}

//now you can do dynamic programming, look thinks up on fly
bool CalculateOptimalROI::imgPixel(int i, int j)
{
    if(testedPixels[j*o_optimalSize.x+i]==0)
    {
        //check that pixel at each place
        for(UIntSet::const_iterator it=activeImages.begin();it!=activeImages.end();it++)
        {
            double xd,yd;
            if(transfMap[*it]->transformImgCoord(xd,yd,(double)i,(double)j))
            {
                if(o_panorama.getImage(*it).isInside(vigra::Point2D(xd,yd)))
                {
                    //if found in a single image, short cut out
                    testedPixels[j*o_optimalSize.x+i]=1;
                    pixels[j*o_optimalSize.x+i]=1;
                    return true; 
                }
            }
        }
        
        //if made it through the for loop without a success, mark as bad
        testedPixels[j*o_optimalSize.x+i]=1;
        pixels[j*o_optimalSize.x+i]=0;
        return false;
    }
    //else it is know if this pixel is covered by at least one image
    else
    {
        return pixels[j*o_optimalSize.x+i];
    }
}

void CalculateOptimalROI::makecheck(int left,int top,int right,int bottom)
{
    if(left<max.left || top<max.top || right>max.right || bottom>max.bottom)
    {
        return;
    }
    
    if(left<right && top<bottom)
    {
        //big enough
#ifdef USEAREA
        if(maxvalue>0 && (right-left)*(bottom-top)<maxvalue)
#else
        if(maxvalue>0 && (right-left)+(bottom-top)<maxvalue)
#endif
        {
            return;
        }
        
        nonrec *tmp=head;
        //no delete test
        //nonrec *tmp=begin;
        //check if exists
        while(tmp!=NULL)
        {
            if(tmp->left==left && tmp->right==right && tmp->top==top && tmp->bottom==bottom)
            {
                //printf("found dupe\n");
                return;
            }
            tmp=tmp->next;
        }
        
        //make
        tmp=new nonrec;
        tmp->left=left;
        tmp->top=top;
        tmp->right=right;
        tmp->bottom=bottom;
        tmp->next=NULL;
        
        count++;
        tail->next=tmp;
        tail=tmp;
    }
    return;
}

void CalculateOptimalROI::nonreccheck(int left,int top,int right,int bottom,int acc,int dodouble)
{
    nonrec *tmp;
    tmp=new nonrec;
    tmp->left=left;
    tmp->top=top;
    tmp->right=right;
    tmp->bottom=bottom;
    tmp->next=NULL;
    
    begin=tmp;
    head=tmp;
    tail=tmp;
    count=0;
    count++;
    
    while(count>0)
    {
        count--;
        left=head->left;
        top=head->top;
        right=head->right;
        bottom=head->bottom;
        
        //do lasso, check if failed
        int i,j,flag;
        flag=0;
        for(i=left;i<right && flag==0;i++)
        {
            if(imgPixel(i,top)==0 || imgPixel(i,bottom-1)==0)
            {
                flag=1;
            }
        }
        
        for(j=top;j<bottom && flag==0;j++)
        {
            if(imgPixel(left,j)==0 || imgPixel(right-1,j)==0)
            {
                flag=1;
            }
        }

        //if failed, then recurse
        
        if(dodouble==1)
        {
            if(flag==1)
            {
                //all directions (shrink only)
                makecheck(left,top+acc,right,bottom);
                makecheck(left,top,right,bottom-acc);
                makecheck(left+acc,top,right,bottom);
                makecheck(left,top,right-acc,bottom);
                
            }
            //it was good, stop recursing
            else
            {
                //printf("\nGood\n");
#ifdef USEAREA
                if(maxvalue<(right-left)*(bottom-top))
#else
                if(maxvalue<(right-left)+(bottom-top))
#endif
                {
#ifdef USEAREA
                    maxvalue=(right-left)*(bottom-top);
#else
                    maxvalue=(right-left)+(bottom-top);
#endif
                    best.right=right;
                    best.left=left;
                    best.top=top;
                    best.bottom=bottom;
                }
            }
        }
        else
        {
            if(flag==0)
            {
                makecheck(left-acc,top,right,bottom);
                makecheck(left,top,right+acc,bottom);
                makecheck(left,top-acc,right,bottom);
                makecheck(left,top,right,bottom+acc);
#ifdef USEAREA
                if(maxvalue<(right-left)*(bottom-top))
#else
                if(maxvalue<(right-left)+(bottom-top))
#endif
                {
#ifdef USEAREA
                    maxvalue=(right-left)*(bottom-top);
#else
                    maxvalue=(right-left)+(bottom-top);
#endif
                    best.right=right;
                    best.left=left;
                    best.top=top;
                    best.bottom=bottom;
                }
            }
        }
        
        tmp=head->next;
        if(tmp!=NULL)
        {
            //experiment with no deletion
            delete head;
            head=tmp;
        }
    }
    
    //no delete test 
    while(head!=NULL)
    {
        tmp=head->next;
        delete head;
        head=tmp;
    }
    
    if(maxvalue>0 && acc==1 && dodouble==0)
    {
        printf("Found Solution: %d %d %d %d\n",best.left,best.top,best.right,best.bottom);
    }
}

int CalculateOptimalROI::autocrop()
{
    printf("Original Image: %dx%d\n",o_optimalSize.x,o_optimalSize.y);
    
    maxvalue=0;
    count=0;
    begin=NULL;
    head=NULL;
    tail=NULL;

    //put backwards at the start
    min.right=0;
    min.bottom=0;
    min.left=o_optimalSize.x;
    min.top=o_optimalSize.y;
    
    max.left=0;
    max.top=0;
    max.right=o_optimalSize.x;
    max.bottom=o_optimalSize.y;
    
    int startacc=pow(2.0,std::min((int)log2(o_optimalSize.x/2-1),(int)log2(o_optimalSize.y/2-1))-1);
    if(startacc<1)
        startacc=1;
    
    //start smaller to get biggest initial position
    for(int acc=startacc;acc>=1;acc/=2)
    {
        nonreccheck(0,0,o_optimalSize.x,o_optimalSize.y,acc,1);
        if(maxvalue>0)
        {
            printf("Inner %d %d: %d %d - %d %d\n",acc,maxvalue,best.left,best.right,best.top,best.bottom);
            min=best;
            break;
        }
    }
    
    for(int acc=startacc;acc>=1;acc/=2)
    {
        printf("Starting %d: %d %d - %d %d\n",acc,best.left,best.right,best.top,best.bottom);
        nonreccheck(best.left,best.top,best.right,best.bottom,acc,0);
        min=best;
    }

    //printf("Final Image: %dx%d\n",outpano->width,outpano->height);
    return 0;
}

} //namespace
