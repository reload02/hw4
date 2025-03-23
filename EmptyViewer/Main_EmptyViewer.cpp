#include <Windows.h>
#include <iostream>
#include <GL/glew.h>
#include <GL/GL.h>
#include <GL/freeglut.h>

#define GLFW_INCLUDE_GLU
#define GLFW_DLL
#include <GLFW/glfw3.h>
#include <vector>

#define GLM_SWIZZLE
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

using namespace glm;

// -------------------------------------------------
// Global Variables
// -------------------------------------------------
int Width = 512;
int Height = 512;
std::vector<float> OutputImage;
// -------------------------------------------------

struct Ray {
	vec3 origin;
	vec3 direction;
};

struct Intersection {
	bool hit;
	float distance;
};
class Surface {
public:
	virtual ~Surface() {}
	virtual Intersection intersect(const Ray& ray) const = 0;
};

class Plane : public Surface {
public:
	vec3 normal; //법선 벡터
	float d; // 상수

	Plane(vec3 n, float d_) : normal(glm::normalize(n)), d(d_) {}


	Intersection intersect(const Ray& ray) const override {
		float denom = glm::dot(normal, ray.direction);
		if (fabs(denom) > 1e-6) {
			float t = -(glm::dot(normal, ray.origin) + d) / denom;
			if (t >= 0.001f) return { true, t };
		}
		return { false, 0 };// 광선과 평면이 만나는지 여부 계산 + 거리 계산
	}
};

class Sphere : public Surface {
public:
	vec3 center;
	float radius;

	Sphere(const vec3& c, float r) : center(c), radius(r) {}


	Intersection intersect(const Ray& ray) const override {
		glm::vec3 oc = ray.origin - center;
		float a = glm::dot(ray.direction, ray.direction);
		float b = 2.0f * glm::dot(oc, ray.direction);
		float c = glm::dot(oc, oc) - radius * radius;
		float discriminant = b * b - 4 * a * c;

		if (discriminant < 0) return { false, 0 };
		else {
			float t = (-b - glm::sqrt(discriminant)) / (2.0f * a);
			if (t > 0.001f) return { true, t };
			else return { false, 0 };
		}
	}// 구와 평면이 만나는지 여부 계산 + 거리 계산
};

class Camera {
public:

	vec3 eye;
	vec3 u, v, w;
	float l=-0.1, r=0.1, b=-0.1, t=0.1, d=0.1;

	Camera(const vec3& e, const vec3& u, const vec3& v, const vec3& w) : eye(e), u(u),v(v),w(w) {}

	Ray getRay(int i, int j) const {
		float u = l + (r - l) * (i + 0.5f) / Width;
		float v = b + (t - b) * (j + 0.5f) / Height;
		return { eye, glm::normalize(glm::vec3(u,v,-d)) };
	} // 해당 위치에서 카메라가 볼수있는  광선 계산

};






void render()
{
	float max = -1;
	float min = 100;

	//Create our image. We don't want to do this in 
	//the main loop since this may be too slow and we 
	//want a responsive display of our beautiful image.
	//Instead we draw to another buffer and copy this to the 
	//framebuffer using glDrawPixels(...) every refresh


	// 필요한 객체 생성

	Surface* s1 = new Sphere(vec3(-4.0f, 0.0f, -7.0f), 1.0f);
	Surface* s2 = new Sphere(vec3(0.0f, 0.0f, -7.0f), 2.0f);
	Surface* s3 = new Sphere(vec3(4.0f, 0.0f, -7.0f), 1.0f);
	Surface* p1 = new Plane(vec3( 0,1,0 ), 2);

	Camera* c = new Camera(vec3(0, 0, 0), vec3(1, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1)); 


	OutputImage.clear();
	for (int j = 0; j < Height; ++j) 
	{
		for (int i = 0; i < Width; ++i) 
		{
			
			// ---------------------------------------------------
			// --- Implement your code here to generate the image
			// ---------------------------------------------------

			// draw a red rectangle in the center of the image
			vec3 color = glm::vec3(0.0f, 0.0f, 0.0f); // grey color [0,1] in RGB channel
			
			
			Ray ray = c->getRay(i, j);
			

			/*max = s1->intersect(ray).distance > max ? s1->intersect(ray).distance : max;
			min = s1->intersect(ray).distance > 0 ? s1->intersect(ray).distance < min ? s1->intersect(ray).distance : min : min;*/


			// 광선과 물체가 부딪히는지 확인 후 색 부여

			if (s1->intersect(ray).hit)color = glm::vec3(1.0f, 1.0f, 1.0f);
			if (s2->intersect(ray).hit)color = glm::vec3(1.0f, 1.0f, 1.0f);
			if (s3->intersect(ray).hit)color = glm::vec3(1.0f, 1.0f, 1.0f);
			if (p1->intersect(ray).hit)color = glm::vec3(1.0f, 1.0f, 1.0f);
			
			// set the color
			OutputImage.push_back(color.x); // R
			OutputImage.push_back(color.y); // G
			OutputImage.push_back(color.z); // B

			
			
			
		}
	}
	
	delete s1;
	delete s2;
	delete s3;
	delete p1;
	delete c;   // 사용한 객체 제거
	//std::cout <<max<<"    "<<min;
}


void resize_callback(GLFWwindow*, int nw, int nh) 
{
	//This is called in response to the window resizing.
	//The new width and height are passed in so we make 
	//any necessary changes:
	Width = nw;
	Height = nh;
	//Tell the viewport to use all of our screen estate
	glViewport(0, 0, nw, nh);

	//This is not necessary, we're just working in 2d so
	//why not let our spaces reflect it?
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho(0.0, static_cast<double>(Width)
		, 0.0, static_cast<double>(Height)
		, 1.0, -1.0);

	//Reserve memory for our render so that we don't do 
	//excessive allocations and render the image
	OutputImage.reserve(Width * Height * 3);
	render();
}


int main(int argc, char* argv[])
{

	

	// -------------------------------------------------
	// Initialize Window
	// -------------------------------------------------

	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(Width, Height, "OpenGL Viewer", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	//We have an opengl context now. Everything from here on out 
	//is just managing our window or opengl directly.

	//Tell the opengl state machine we don't want it to make 
	//any assumptions about how pixels are aligned in memory 
	//during transfers between host and device (like glDrawPixels(...) )
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);

	//We call our resize function once to set everything up initially
	//after registering it as a callback with glfw
	glfwSetFramebufferSizeCallback(window, resize_callback);
	resize_callback(NULL, Width, Height);

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		//Clear the screen
		glClear(GL_COLOR_BUFFER_BIT);

		// -------------------------------------------------------------
		//Rendering begins!
		glDrawPixels(Width, Height, GL_RGB, GL_FLOAT, &OutputImage[0]);
		//and ends.
		// -------------------------------------------------------------

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();

		//Close when the user hits 'q' or escape
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS
			|| glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}
