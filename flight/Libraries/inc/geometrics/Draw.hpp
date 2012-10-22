
#ifndef DRAW3D_HPP
#define DRAW3D_HPP

#include "opencv/cv.h"
#include "geometrics/Segment.hpp"
#include <exception>


class drawexception : public std::exception
{
	virtual const char* what() const throw()
	{
		return "Cannot project point with negative z coordinate";
	}
} drawex;

class Draw3d {

public:
	cv::Vec2i center;
	cv::Vec2d focal;
	cv::Vec3d distortion;
	cv::Mat camera;

	Draw3d(cv::Vec2i _center, cv::Vec2d _focal,cv::Vec3d _distortion,cv::Mat _camera) {
		center=_center;
		focal=_focal;
		distortion=_distortion;
		camera=_camera;
	}

	cv::Point2i project(cv::Point3d orig) {

		if (orig.z<=0) throw drawex;
		cv::Vec2d p(orig.x/orig.z,orig.y/orig.z);
		double r2=p[0]*p[0]+p[1]*p[1];
		double factor=1+distortion[0]*r2+distortion[1]*r2*r2+distortion[2]*r2*r2*r2;
		p *= factor;
		return cv::Point2i(center[0]+focal[0]*p[0],center[1]+focal[1]*p[1]);

	}

	void drawLine(cv::Mat& img, cv::Point3d A,cv::Point3d B,const Scalar& color, int thickness=1, int lineType=8, int shift=0) {
		cv::Mat V1(cv::Mat(A)*camera);
		cv::Mat V2(cv::Mat(B)*camera);
		drawLineLocal(img, cv::Point3d(V1), cv::Point3d(V2), color, thickness, lineType, shift);
	}

	void drawLineLocal(cv::Mat& img, cv::Point3d A,cv::Point3d B, const Scalar& color, int thickness=1, int lineType=8, int shift=0) {
		geo::Segment l(A,B);
		if (!l.useable) return;
		// cut negative z component
		l=l.cut(geo::Plane(geo::Line(cv::Point3d(0,0,0),cv::Vec3d(0,0,1))));
		if (!l.useable) return;

		cv::Point3d c(0,0,0);
		cv::Point3d ul(-center[0]/focal[0],-center[1]/focal[1],1.);
		cv::Point3d ur(center[0]/focal[0],-center[1]/focal[1],1.);
		cv::Point3d bl(-center[0]/focal[0],center[1]/focal[1],1.);
		cv::Point3d br(center[0]/focal[0],center[1]/focal[1],1.);
		
		// cut parts above image
		l=l.cut(geo::Plane(c,ul,ur));
		if (!l.useable) return;
		// cut parts below image
		l=l.cut(geo::Plane(c,br,bl));
		if (!l.useable) return;
		// cut parts left of image
		l=l.cut(geo::Plane(c,bl,ul));
		if (!l.useable) return;
		// cut parts right of image
		l=l.cut(geo::Plane(c,ur,br));
		if (!l.useable) return;

		// traverse line in 10 pixel steps
		cv::Vec2d a(cv::Vec2i(project(l.P0))),b(cv::Vec2i(project(l.P1)));
		int length=norm(b-a);
		int steps=length/10;
		if (!steps) steps=1;

		for (int t=0;t<steps;t++) {
			// calculate beginning and end points of line segment
			// 2d
			cv::Vec2d p0(a+t*(b-a));
			cv::Vec2d p1(a+t*(b-a));
			// rough aproximation 3d (without distortion)
			cv::Vec3d q0((p0[0]-center[0])/focal[0],(p0[1]-center[1])/focal[1],1);
			cv::Vec3d q1((p1[0]-center[0])/focal[0],(p1[1]-center[1])/focal[1],1);
			geo::Segment s=l.cut(geo::Line(q0,cv::Vec3d(B-A)));
			s=s.cut(geo::Line(q1,cv::Vec3d(A-B)));
			if (s.useable) {
				cv::line(img,project(s.P0),project(s.P1),color,thickness,lineType,shift);
			}
		}


	}

};


#endif
