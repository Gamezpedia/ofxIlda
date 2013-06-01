//
//  ofxIldaFrame.h
//  ofxIlda
//
//  Created by Memo Akten on 09/05/2013.
//
//


// a single ILDA frame, contains multiple polys
// coordinates are NORMALIZED (0...1, 0...1)

#pragma once

#include "ofMain.h"
#include "ofxIldaPoint.h"

namespace ofxIlda {
	
    class Frame {
    public:
        struct {
            struct {
                int smoothAmount;   // how much to smooth the path (zero to ignore)
                float optimizeTolerance;    // howmuch to optimize the path, based on curvature (zero to ignore)
                bool collapse;  // (not implemented yet)
                int targetPointCount;   // how many points in total should ALL paths in this frame be resampled to (zero to ignore)
                float spacing;  // desired spacing between points. Set automatically by targetPointCount, or set manually. (zero to ignore)
            } path;
            
            struct {
                bool lines; // draw lines
                bool points;    // draw points
                bool pointNumbers;  // draw point numbers (not implemented yet)
            } draw;
            
            struct {
                ofFloatColor color; // color
                int blankCount;     // how many blank points to send at path ends
                int endCount;       // how many end repeats to send
                bool doCapX;        // cap out of range on x (otherwise wraps around)
                bool doCapY;        // cap out of range on y (otherwise wraps around)
                struct {
                    bool doFlipX;
                    bool doFlipY;
                    ofVec2f offset;
                    ofVec2f scale;
                } transform;
            } output;
        } params;
        
        struct {
            int pointCountOrig;    // READONLY current total number of points across all paths (excluding blanks and end repititons)
            int pointCountProcessed; // same as above, except AFTER being processed
        } stats;
        
        
        //--------------------------------------------------------------
        Frame() {
            setDefaultParams();
        }
        
        //--------------------------------------------------------------
        void setDefaultParams() {
            memset(&params, 0, sizeof(params));  // safety catch all default to zero
            memset(&stats, 0, sizeof(stats));  // safety catch all default to zero
            
            params.path.smoothAmount = 0;
            params.path.optimizeTolerance = 0;
            params.path.collapse = 0;
            params.path.targetPointCount = 500;
            params.path.spacing = 0;
            
            params.draw.lines = true;
            params.draw.points = true;
            params.draw.pointNumbers = false;
            
            params.output.color.set(1, 1, 1, 1);
            params.output.blankCount = 30;
            params.output.endCount = 30;
            params.output.doCapX = false;
            params.output.doCapY = false;
            
            params.output.transform.doFlipX = false;
            params.output.transform.doFlipY = false;
            params.output.transform.offset.set(0, 0);
            params.output.transform.scale.set(1, 1);
        }
        
        
        //--------------------------------------------------------------
        string getParams() {
            stringstream s;
            s << "params:" << endl;
            s << "path.smoothAmount : " << params.path.smoothAmount << endl;
            s << "path.optimizeTolerance : " << params.path.optimizeTolerance << endl;
            s << "path.collapse : " << params.path.collapse << endl;
            s << "path.targetPointCount : " << params.path.targetPointCount << endl;
            s << "path.spacing : " << params.path.spacing << endl;
            
            s << "draw.lines : " << params.draw.lines << endl;
            s << "draw.point : " << params.draw.points << endl;
            s << "draw.pointNumbers : " << params.draw.pointNumbers << endl;
            
            s << "output.color : " << params.output.color << endl;
            s << "output.blankCount : " << params.output.blankCount << endl;
            s << "output.endCount : " << params.output.endCount << endl;
            s << "output.doCapX : " << params.output.doCapX << endl;
            s << "output.doCapY : " << params.output.doCapY << endl;
            s << "output.transform.doFlipX : " << params.output.transform.doFlipX << endl;
            s << "output.transform.doFlipY : " << params.output.transform.doFlipY << endl;
            s << "output.transform.offset : " << params.output.transform.offset << endl;
            s << "output.transform.scale : " << params.output.transform.scale << endl;
            
            s << endl;
            
            s << "stats:" << endl;
            s << "stats.pointCountOrig : " << stats.pointCountOrig << endl;
            s << "stats.pointCountProcessed : " << stats.pointCountProcessed << endl;
            
            return s.str();
        }
        
        //--------------------------------------------------------------
        void update() {
            float totalLength = 0;
            vector<int> pathLengths;
            processedPolys = origPolys;
            for(int i=0; i<processedPolys.size(); i++) {
                if(processedPolys[i].size()) {
                    // smooth paths
                    if(params.path.smoothAmount > 0) processedPolys[i] = processedPolys[i].getSmoothed(params.path.smoothAmount);
                    
                    // optimize paths
                    if(params.path.optimizeTolerance > 0) processedPolys[i].simplify(params.path.optimizeTolerance);
                    
                    // calculate total length (needed for auto spacing calculation)
                    if(params.path.targetPointCount > 0) {
                        float l = processedPolys[i].getPerimeter();
                        totalLength += l;
                        pathLengths.push_back(l);
                    }
                } else {
                    pathLengths.push_back(0);
                }
            }
            
            
            // calculate spacing based on desired total number of points
            if(params.path.targetPointCount > 0 && totalLength > 0) {
                params.path.spacing = totalLength / params.path.targetPointCount;
            }
            
            
            // resample paths based on spacing (either as calculated by targetPointCount, or set by user)
            if(params.path.spacing) {
                for(int i=0; i<processedPolys.size(); i++) {
                    processedPolys[i] = processedPolys[i].getResampledBySpacing(params.path.spacing);
                }
            }
            
            
            // get stats
            stats.pointCountOrig = 0;
            stats.pointCountProcessed = 0;
            for(int i=0; i<processedPolys.size(); i++) {
                stats.pointCountOrig += origPolys[i].size();
                stats.pointCountProcessed += processedPolys[i].size();
            }
            
            updateFinalPoints();
        }
        
        
        //--------------------------------------------------------------
        void draw(float x=0, float y=0, float w=0, float h=0) {
            if(w==0) w = ofGetWidth();
            if(h==0) h = ofGetHeight();
            
            ofPushStyle();
            ofPushMatrix();
            ofTranslate(x, y);
            ofScale(w, h);
            
            if(params.draw.lines) {
                ofSetLineWidth(2);
                for(int i=0; i<processedPolys.size(); i++) {
                    ofPolyline &poly = processedPolys[i];
                    poly.draw();
                    //            for(int i=0; i<data.size(); i++) {
                    //                ofPoint p0 = data[i];
                    //                if(i < data.size()-1) {
                    //                    ofPoint p1 = data[i+1];
                    ////                    ofSetColor(p1.r * 255, p1.g * 255, p1.b * 255, p1.a * 255);
                    //                    ofLine(p0.x, p0.y, p1.x, p1.y);
                    //                }
                    //            }
                }
            }
            if(params.draw.points) {
                glPointSize(5);
                for(int i=0; i<processedPolys.size(); i++) {
                    ofPolyline &poly = processedPolys[i];
                    glBegin(GL_POINTS);
                    for(int i=0; i<poly.size(); i++) {
                        ofPoint &p = poly[i];
                        //                Point &p = data[i];
                        //                ofSetColor(p.r * 255, p.g * 255, p.b * 255, p.a * 255);
                        glVertex2f(p.x, p.y);
                    }
                    glEnd();
                }
            }
            
            ofPopMatrix();
            ofPopStyle();
        }
        
        //--------------------------------------------------------------
        void clear() {
            origPolys.clear();
            processedPolys.clear();
			processedPolysColor.clear();
        }
        
        //--------------------------------------------------------------
        int size() {
            return origPolys.size();
        }
        
        //--------------------------------------------------------------
        ofPolyline& addPoly() {
			return addPoly(ofPolyline());
        }
        
        //--------------------------------------------------------------
        ofPolyline& addPoly(const ofPolyline& poly) {
            origPolys.push_back(poly);
			processedPolysColor.push_back(params.output.color);
            return origPolys.back();
        }
        
        //--------------------------------------------------------------
        ofPolyline& addPoly(const vector<ofPoint> points) {
			return addPoly(ofPolyline(points));
        }
        
        //--------------------------------------------------------------
        ofPolyline& getPoly(int i) {
            return origPolys[i];
        }
        
        //--------------------------------------------------------------
        ofPolyline& getPolyProcessed(int i) {
            return processedPolys[i];
        }
        
        
        //--------------------------------------------------------------
        ofPolyline& getLastPoly() {
            if(origPolys.empty()) addPoly();
            return origPolys.back();
        }
        
        
        //--------------------------------------------------------------
        const vector<Point>& getPoints() const {
            return points;
        }
        
        //--------------------------------------------------------------
        ofPoint transformPoint(ofPoint p) const {
            // flip
            if(params.output.transform.doFlipX) p.x = 1 - p.x;
            if(params.output.transform.doFlipY) p.y = 1 - p.y;
            
            // scale
            if(params.output.transform.scale.lengthSquared() > 0) {
                p -= ofPoint(0.5, 0.5);
                p *= params.output.transform.scale;
                p += ofPoint(0.5, 0.5);
            }
            
            // offset
            p += params.output.transform.offset;
            


            
            // cap or wrap
            if(p.x < 0) {
                p.x = params.output.doCapX ? 0 : 1 + p.x - ceil(p.x);
            } else if(p.x > 1) {
                p.x = params.output.doCapX ? 1 : p.x - floor(p.x);
            }
            
            if(p.y < 0) {
                p.y = params.output.doCapY ? 0 : 1 + p.y - ceil(p.y);
            } else if(p.y > 1) {
                p.y = params.output.doCapY ? 1 : p.y - floor(p.y);
            }
            
            return p;
        }
        
        //--------------------------------------------------------------
        void updateFinalPoints() {
            points.clear();
			
            for(int i=0; i<processedPolys.size(); i++) {
                ofPolyline &poly = processedPolys[i];
                ofFloatColor &pcolor = processedPolysColor[i];
                if(poly.size() > 0) {
					                    ofPoint startPoint = transformPoint(poly.getVertices().front());
                    ofPoint endPoint = transformPoint(poly.getVertices().back());
                    
                    // blanking at start
                    for(int n=0; n<params.output.blankCount; n++) {
                        points.push_back( Point(startPoint, ofFloatColor(0, 0, 0, 0)));
                    }
                    
                    // repeat at start
                    for(int n=0; n<params.output.endCount; n++) {
						points.push_back( Point(startPoint, pcolor) );
                    }
                    
                    // add points
                    for(int j=0; j<poly.size(); j++) {
						points.push_back( Point(transformPoint(poly[j]), pcolor) );
                    }
                    
                    // repeat at end
                    for(int n=0; n<params.output.endCount; n++) {
						points.push_back( Point(endPoint, pcolor) );
                    }
                    
                    // blanking at end
                    for(int n=0; n<params.output.blankCount; n++) {
                        points.push_back( Point(endPoint, ofFloatColor(0, 0, 0, 0) ));
                    }
                    
                }
            }
        }
        
    protected:
        vector<ofPolyline> origPolys;   // stores the original polys
        vector<ofPolyline> processedPolys;  // stores the processed (smoothed, collapsed, optimized, resampled etc).
        vector<Point> points;   // final points to send
        vector<ofFloatColor> processedPolysColor;	// color per poly
        
    };
}