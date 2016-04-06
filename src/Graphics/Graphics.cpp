/*
 * Graphics.cpp
 *
 *  Created on: Oct 28, 2015
 *      Author: Admin
 */
#define FLEQUAL(a,b) (std::abs(a - b) < 0.001)
#include "Graphics.h"
#include <SDL2/SDL.h>
#include <execinfo.h>

#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#include <array>
#include <functional>

//General graphics flow:

Graphics::Graphics(uint32_t width, uint32_t height, const char *name) {
	if (!SDL_WasInit(SDL_INIT_VIDEO)) {
		SDL_Init(SDL_INIT_VIDEO);
	}
	this->width = width;
	this->height = height;

	this->window = SDL_CreateWindow(name,
			SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height,
			SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN);
	if (!this->window) {
		fprintf(stderr, "Error creating window, %s\n", SDL_GetError());
	}
	assert(window != nullptr);

	this->renderer = SDL_CreateRenderer(this->window, -1,
			SDL_RENDERER_ACCELERATED);
	if (!this->renderer) {
		fprintf(stderr,"Error creating renderer: %s", SDL_GetError());
	}
	assert(renderer!= nullptr);

	auto screen_dist = 100;
	auto hypot = sqrt(SQ(width/2)+SQ(screen_dist));
	auto cos_horiz_angle = 2*acos(screen_dist/hypot);
	auto sin_horiz_angle = 2*asin(width/(2*hypot));
	assert(FLEQUAL(cos_horiz_angle,sin_horiz_angle));

	horizontal_angle = cos_horiz_angle;

	hypot = sqrt(SQ(height/2)+SQ(screen_dist));
	cos_horiz_angle = 2*acos(screen_dist/hypot);
	sin_horiz_angle = 2*asin(height/(2*hypot));
	assert(FLEQUAL(cos_horiz_angle,sin_horiz_angle));

	vertical_angle = cos_horiz_angle;

}

void Graphics::Clear() {
	SetColor(Color::White);
	SDL_RenderClear(this->renderer);
}

void Graphics::Update() {
	SDL_UpdateWindowSurface(this->window);
	SDL_RenderPresent(this->renderer);

}

void Graphics::ProjectVec3(const Vec3 &v, const Color &c, int scalar) const{
	this->SpaceLine(0, 0, v.dot(Vec3::I) * scalar, v.dot(Vec3::J) * scalar, c);
}

void Graphics::LineFromVec(const Vec3 &v1, const Vec3 &v2, const Color &c) const {
	this->SpaceLine(v1.x, v1.y, v2.x, v2.y, c);
}

void Graphics::SpaceLine(int32_t x1, int32_t y1, int32_t x2, int32_t y2,const Color &c) const {
	Line(this->width / 2 + x1, this->height / 2 - y1, this->width / 2 + x2,
			this->height / 2 - y2, c);
}

void Graphics::Line(int32_t x1, int y1, int x2, int y2, const Color &c) const {
	SetColor(c);
	SDL_RenderDrawLine(this->renderer, x1, y1, x2, y2);
}



void Graphics::PutPixel(int x, int y, const Color &c) const {
	if (!(x > this->width && x < 0 && y > this->height && y < 0)) {
		SetColor(c);
		SDL_RenderDrawPoint(this->renderer, x, y);
	} else {
//		fprintf(stderr,
//				"Error putting pixel at (%d,%d) with color (%d,%d,%d)\n", x, y,
//				c.r, c.g, c.b);
	}
}

Graphics::~Graphics() {
	// TODO Auto-generated destructor stub
	//	SDL_FreeSurface(surface);
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(this->renderer);
	SDL_Quit();
}

void Graphics::SetColor(const Color &c) const {
	SDL_SetRenderDrawColor(this->renderer, c.r, c.g, c.b, 255);

}

Color Graphics::SDLColorToColor(uint32_t n) const{
	Color c = Color(0,0,0);
	c.r = n & 0xff000000;
	c.g = n & 0xff0000;
	c.b = n & 0xff00;
	return c;
}

Vec3 GetBarycentric(const Vec3 &t0, const Vec3 &t1, const Vec3 &t2, const Vec3 &p){
	(void) t0;
	(void) t1;
	(void) t2;
	(void) p;
	Vec3 as_bary;
	return as_bary;
}

//https://github.com/ssloy/tinyrenderer/wiki/Lesson-2:-Triangle-rasterization-and-back-face-culling
void Graphics::Triangle(const std::array<Vec4,3>& tri, const Color& c, const Color &fill) const{

	std::array<Vec4,3> corrected = tri;
	(void) fill;
	Mat4 proj = Mat4::Projection();

	for(auto& v : corrected){
		auto scale = v.z;
		v = (proj * v) / scale;
	}

	auto t0 = corrected[0];
	auto t1 = corrected[1];
	auto t2 = corrected[2];

	//sort the three in order

	if (t0.y>t1.y) std::swap(t0, t1); 
	if (t0.y>t2.y) std::swap(t0, t2); 
	if (t1.y>t2.y) std::swap(t1, t2); 

	//draw lines
	LineFromVec(t0,t1,c);
	LineFromVec(t1,t2,c);
	LineFromVec(t2,t0,c);

	//get bounding box for rasterizing
}

void Graphics::Triangle(const std::array<Vec4,3>& tri, const Color& c) const{
	Triangle(tri,c,c);
}



void Graphics::Polygon(const std::vector<Vertex>& poly, const Color& c) const{
	std::vector<Vertex> corrected = poly;
	Mat4 proj = Mat4::Projection();

	for(auto& v : corrected){
		auto scale = v.pos.z;
		v.pos = (proj * v.pos) / scale;
	}

	for(const auto& vertex : poly){
		for(const auto vert_ref : vertex.adj){
			this->LineFromVec(vertex.pos,poly[vert_ref].pos,c);
		}
	}
}


void Graphics::Polygon(const std::vector<Vertex>& poly, const Color& c, const Quat& rotation) const{
	std::vector<Vertex> corrected = poly;
	Mat4 proj = Mat4::Projection();

	for(auto& v : corrected){
		auto scale = v.pos.z;
		v.pos = (proj * v.pos) / scale;
	}

	for(const auto& vertex : poly){
		for(const auto vert_ref : vertex.adj){
			this->LineFromVec(vertex.pos.rotate(rotation),poly[vert_ref].pos.rotate(rotation),c);
		}
	}
}

void Graphics::Polygon(const std::vector<Vertex>& poly, const Color& c, const std::function<Vec4(Vec4)> transform){
	auto post_transform = poly;
	for(auto& vert : post_transform){
		vert.pos = transform(vert.pos);
	}

	for(const auto& vertex : post_transform){
		for(const auto vert_ref : vertex.adj){
			this->LineFromVec(vertex.pos,post_transform[vert_ref].pos,c);
		}
	}
}

ConventionalPoint::ConventionalPoint(int8_t x, int8_t y){
	assert(INEQ(-1,<=,x,<=,1));
	assert(INEQ(-1,<=,y,<=,1));
	this->x = x;
	this->y = y;
}
