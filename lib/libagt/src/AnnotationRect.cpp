/*
 * Copyright (c) 2014, Intel Corporation
 *
 * All rights reserved.
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions are 
 * met:
 * 
 * - Redistributions of source code must retain the above copyright notice,
 *   this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright 
 *   notice, this list of conditions and the following disclaimer in the 
 *   documentation and/or other materials provided with the distribution.
 * - Neither the name of the Intel Corporation nor the names of its 
 *   contributors may be used to endorse or promote products derived from 
 *   this software without specific prior written permission.
 *  
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A 
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, 
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING 
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "AnnotationRect.h"

AnnotationRect::AnnotationRect(double vx1, double vy1, double vx2, double vy2,AnnotationItem* _parent) : AnnotationItem(_parent)
{
    x1=vx1;
    x2=vx2;
    y1=vy1;
    y2=vy2;
    if (parent!=NULL)
    {
        parent->newChild(this);
    }
}

AnnotationRect::~AnnotationRect()
{
}


bool
AnnotationRect::innerPoint(double x, double y,double scf_x,double scf_y)
{
    if (!selectable) return false;

    double leftX,upperY,rightX,bottomY;

    leftX   = x1<x2 ? x1 : x2;
    rightX  = x1<x2 ? x2 : x1;
    upperY  = y1<y2 ? y1 : y2;
    bottomY = y1<y2 ? y2 : y1;

    // ease case, if the rect is filled => check bounds
    if (myBrush.style()!=Qt::NoBrush)
    {
        bool c1 = (x>=leftX) && (x<=rightX);
        bool c2 = (y>=upperY) && (y<=bottomY);
        return (c1&&c2);
    }
    else
    {
        bool fnd;
        // check near segment for each segment
        fnd = pointNearSegment(x,y,leftX,upperY,leftX,bottomY,scf_x,scf_y);
        if (!fnd) fnd = pointNearSegment(x,y,rightX,upperY,rightX,bottomY,scf_x,scf_y);
        if (!fnd) fnd = pointNearSegment(x,y,leftX,upperY,rightX,upperY,scf_x,scf_y);
        if (!fnd) fnd = pointNearSegment(x,y,leftX,bottomY,rightX,bottomY,scf_x,scf_y);
        return (fnd);
    }
}

bool
AnnotationRect::innerNodePoint(double x, double y,double scf_x,double scf_y)
{
    //printf ("innerNodePoint called x=%f,y=%f\n",x,y);fflush(stdout);
    //printf ("\t line is (%f,%f)->(%f,%f)\n",x1,y1,x2,y2);fflush(stdout);
    if (!selected)
    {
        return (false);
    }    

    // check area arround node selectors
    double leftX,upperY,rightX,bottomY;
    leftX   = x1<x2 ? x1 : x2;
    rightX  = x1<x2 ? x2 : x1;
    upperY  = y1<y2 ? y1 : y2;
    bottomY = y1<y2 ? y2 : y1;

    double node_gap_v = NODE_GAP;
    double node_gap_h = NODE_GAP;
    node_gap_v /= scf_y;
    node_gap_h /= scf_x;

    bool c1 = innerNodePointArround(x,y,scf_x,scf_y,floor(leftX-node_gap_h),floor(upperY-node_gap_v));
    bool c2 = innerNodePointArround(x,y,scf_x,scf_y,floor(rightX+node_gap_h),floor(bottomY+node_gap_v));
    bool c3 = innerNodePointArround(x,y,scf_x,scf_y,floor(leftX-node_gap_h),floor(bottomY+node_gap_v));
    bool c4 = innerNodePointArround(x,y,scf_x,scf_y,floor(rightX+node_gap_h),floor(upperY-node_gap_v));

    //printf("\t c1=%d, c2=%d\n",(INT32)c1,(INT32)c2);fflush(stdout);
    return (c1||c2||c3||c4);
}


void
AnnotationRect::boundingRect(double* rx1, double* ry1, double* w, double* h) const
{
    if (x1<x2)
    {
        *rx1 = x1;
    }    
    else
    {
        *rx1 = x2;
    }    

    if (y1<y2)
    {
        *ry1 = y1;
    }    
    else
    {
        *ry1 = y2;
    }    

    *w  = fabs(x2-x1+1);
    *h  = fabs(y2-y1+1);
}

void
AnnotationRect::draw(QPainter* p, double scf_x, double scf_y)
{
    if (!visible)
    {
        return;
    }    

    //printf ("AnnotationRect::draw called, x1=%f,y1=%f,x2=%f,y2=%f\n",x1,y1,x2,y2);
    double leftX,upperY,rightX,bottomY;
    leftX   = x1<x2 ? x1 : x2;
    rightX  = x1<x2 ? x2 : x1;
    upperY  = y1<y2 ? y1 : y2;
    bottomY = y1<y2 ? y2 : y1;

    QBrush old = p->brush();

    p->setBrush(myBrush);
    p->setPen(myPen);

    // draw the base line
    p->drawRect((int)floor(leftX),(int)floor(upperY),
    (int)floor(rightX-leftX+1.0),(int)floor(bottomY-upperY+1.0));

    // draw (if needed)
    if (selected)
    {
        double node_gap_v = NODE_GAP;
        double node_gap_h = NODE_GAP;
        node_gap_v /= scf_y;
        node_gap_h /= scf_x;

        drawSelectionNode(floor(leftX-node_gap_h),floor(upperY-node_gap_v),p,scf_x,scf_y);
        drawSelectionNode(floor(leftX-node_gap_h),floor(bottomY+node_gap_v),p,scf_x,scf_y);
        drawSelectionNode(floor(rightX+node_gap_h),floor(bottomY+node_gap_v),p,scf_x,scf_y);
        drawSelectionNode(floor(rightX+node_gap_h),floor(upperY-node_gap_v),p,scf_x,scf_y);
    }

    p->setBrush(old);
}

void
AnnotationRect::drawRubber(int px, int py, int x, int y, QPainter* p)
{
    p->drawRect(px,py,x-px,y-py);
}

void
AnnotationRect::mouseWorldPressEvent(int vx, int vy, double wx, double wy)
{
    x1=wx;
    y1=wy;
    //printf ("AnnRect::staring point at %f,%f\n",x1,y1);
}

void
AnnotationRect::mouseWorldReleaseEvent(int vx, int vy, double wx, double wy)
{
    x2=wx;
    y2=wy;
    //printf ("AnnRect::ending point at %f,%f\n",x2,y2);
}

void
AnnotationRect::mouseWorldMoveEvent(int vx, int vy, double wx, double wy)
{
}

AnnotationItem*
AnnotationRect::clone()
{
    AnnotationRect* nl = new AnnotationRect(x1,y1,x2,y2,parent);
    Q_ASSERT(nl!=NULL);
    nl->setPen(myPen);
    nl->setBrush(myBrush);
    return (nl);
}

void
AnnotationRect::clone(AnnotationItem* item)
{
    Q_ASSERT(item!=NULL);
    AnnotationRect* rect = (AnnotationRect*) item;
    rect->x1=x1;
    rect->y1=y1;
    rect->x2=x2;
    rect->y2=y2;
    rect->myPen = myPen;
    rect->myBrush=myBrush;
}

void
AnnotationRect::cloneGeometry(AnnotationItem* item)
{
    Q_ASSERT(item!=NULL);
    AnnotationRect* rect = (AnnotationRect*) item;
    rect->x1=x1;
    rect->y1=y1;
    rect->x2=x2;
    rect->y2=y2;
}

void 
AnnotationRect::move(double x, double y)
{
    x1 += x;
    x2 += x;
    y1 += y;
    y2 += y;
}

void 
AnnotationRect::move(double vx, double vy, double dis)
{
    // normalize the vector
    double mod = sqrt(vx*vx +  vy*vy);
    if (mod == 0.0) 
    {
        return;
    }
        
    vx /= mod;
    vy /= mod;
    x1 += dis*vx;
    y1 += dis*vy;
    x2 += dis*vx;
    y2 += dis*vy;
}

void
AnnotationRect::moveNode(double nx, double ny, double x, double y, double scf_x, double scf_y)
{
    // look for the right node (only two in this case)
    double leftX,upperY,rightX,bottomY;

    leftX   = x1<x2 ? x1 : x2;
    rightX  = x1<x2 ? x2 : x1;
    upperY  = y1<y2 ? y1 : y2;
    bottomY = y1<y2 ? y2 : y1;

    bool swapH = x2<=x1;
    bool swapV = y2<=y1;

    double node_gap_v = NODE_GAP;
    double node_gap_h = NODE_GAP;
    node_gap_v /= scf_y;
    node_gap_h /= scf_x;

    bool c1 = innerNodePointArround(nx,ny,scf_x,scf_y,floor(leftX-node_gap_h),floor(upperY-node_gap_v));
    bool c2 = innerNodePointArround(nx,ny,scf_x,scf_y,floor(rightX+node_gap_h),floor(bottomY+node_gap_v));
    bool c3 = innerNodePointArround(nx,ny,scf_x,scf_y,floor(leftX-node_gap_h),floor(bottomY+node_gap_v));
    bool c4 = innerNodePointArround(nx,ny,scf_x,scf_y,floor(rightX+node_gap_h),floor(upperY-node_gap_v));

    /** @todo review this stupid code! */
    if (c1)
    {
        if (swapH)
            x2+=x;
        else
            x1+=x;

        if (swapV)
            y2+=y;
        else
            y1+=y;
    }
    else if (c2)
    {
        if (swapH)
            x1+=x;
        else
            x2+=x;

        if (swapV)
            y1+=y;
        else
            y2+=y;
    }
    else if (c3)
    {
        if (swapH)
            x2+=x;
        else
            x1+=x;

        if (swapV)
            y1+=y;
        else
            y2+=y;
    }
    else if (c4)
    {
        if (swapH)
            x1+=x;
        else
            x2+=x;
            
        if (swapV)
            y2+=y;
        else
            y1+=y;
    }
}

void AnnotationRect::snapToGrid(int gridSize, bool onlyCorner)
{
    double bx1 = x1;
    double by1 = y1;

    snapToGridPoint(gridSize,x1,y1);
    if (!onlyCorner)
    {
        snapToGridPoint(gridSize,x2,y2);
    }    
    else
    {
        x2 -= (bx1-x1);
        y2 -= (by1-y1);
    }
}


