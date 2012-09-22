
#include <math.h>
#include <stdlib.h>
#include <string.h>

#define PI_2    6.283185307179586476925286766559f
#define PI      3.1415926535897932384626433832795f

float max(const float *f, int len)
{
	float max = f[0];

	while(len--)
	{
		if(f[len] > max)
		{
			max = f[len];
		}
	}

	return max;
}

void convolution(const float *f, int f_w, int f_h, const float *g, int g_w, int g_h, float *h)
{
	int i, j, k, l;

	for(i = g_h/2; i < f_h - g_h/2; i++)
	{
		for(j = g_w/2; j < f_w - g_w/2; j++)
		{
			float sum = 0;

			for(k = 0; k < g_h; k++)
			{
				for(l = 0; l < g_w; l++)
				{
					sum += f[(i + k - g_h/2) * f_w + (j + l - g_w/2)] * g[k * g_w + l];
				}
			}

			h[i * f_w + j] = sum;
		}
	}
}

void norm(const float *f, const float *g, int len, float *h)
{
	while(len--)
	{
		h[len] = sqrtf(f[len] * f[len] + g[len] * g[len]);
	}
}

void normalize_inplace(float *f, int len)
{
	float max_value;

	max_value = max(f, len);

	while(len--)
	{
		f[len] = powf((f[len] / max_value), 0.75f);
	}
}

void sobel(const float *f, int w, int h, float *vert, float *horiz, float *lum)
{
	float filter_sobel_v[] =
	{
		-1, -2, -1,
		 0,  0,  0,
		 1,  2,  1,
	};

	float filter_sobel_h[] =
	{
		-1,  0,  1,
		-2,  0,  2,
		-1,  0,  1,
	};

	convolution(f, w, h, filter_sobel_v, 3, 3, vert);
	convolution(f, w, h, filter_sobel_h, 3, 3, horiz);
	norm(vert, horiz, w*h, lum);
}

void quantize_8bit(const float *f, int len, unsigned char *g)
{
	while(len--)
	{
		g[len] = (unsigned char )(f[len] * 255);
	}
}

void hough(const float *edges, int w, int h, float *hough_space, int theta_res, int r_res)
{
	float max_radius;
	int i, j, k;

	max_radius = sqrtf((float)(w*w+h*h))/2.0f;

	for(i = 0; i < h; i++)
	{
		for(j = 0; j < w; j++)
		{
			int cartesian_offset;
			int x, y;

			/* calculate the current processed edgel address */
			cartesian_offset = i*w+j;
			if(edges[cartesian_offset] < 0.5) continue;
			/* translate the image so that its center is at the origin*/
			x = j-w/2;
			y = i-h/2;

			/* iterating over all posible lines passing through the edgel */
			for(k = 0; k < theta_res; k++)
			{
				float theta, rho;
				int l;
				int hough_offset;

				/* convert theta variable from index presentation to radians */
				theta = k*PI/theta_res;

				/* translate theta to go from -90 to +90 degrees */
				theta -= PI/2;

				/* calculate rho (the distance of the current processed line passing throuh the edgel */
				rho = x*cosf(theta)+y*sinf(theta);

				/* quantizing rho to meet the resolution of the output hough space */
				l = (int)floor(rho*(r_res/2)/max_radius + 0.5f);

				/* translate qunatized rho to go from -r to +r */
				l += r_res/2;

				/* calculate the target bin address */
				hough_offset = l*theta_res+k;

				/* voting with the edgel strength */
				hough_space[hough_offset] += edges[cartesian_offset];
			}
		}
	}
}

extern "C" int lines(unsigned char *in, int w, int h, void (*update)(void *))
{
	static float *f = (float *)malloc(w*h*sizeof(float));
	static float *f_sobel_v = (float *)malloc(w*h*sizeof(float));
	static float *f_sobel_h = (float *)malloc(w*h*sizeof(float));
	static float *f_sobel = (float *)malloc(w*h*sizeof(float));
	static float *f_hough = (float *)malloc(w*h*sizeof(float));
	static unsigned char *out = (unsigned char *)malloc(w*h);
	
	for(int i = 0; i < w*h; i++)
	{
		f[i] = in[i] / 255.0f;
		f_sobel_v[i] = 0.0f;
		f_sobel_h[i] = 0.0f;
		f_sobel[i] = 0.0f;
		f_hough[i] = 0.0f;
	}

	sobel(f, w, h, f_sobel_v, f_sobel_h, f_sobel);
	hough(f_sobel, w, h, f_hough, w, h);

	normalize_inplace(f_sobel, w*h);
	normalize_inplace(f_hough, w*h);

	update(in);

	quantize_8bit(f_sobel, w*h, out);
	update(out);

	quantize_8bit(f_hough, w*h, out);
	update(out);

	return 1;
}
