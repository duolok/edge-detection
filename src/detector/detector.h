#include <iostream>
#include <vector>
#include <string>
#include <tbb/task_group.h>
#include "../bitmap/EasyBMP.h"
#include "../bitmap/BitmapRawConverter.h"

#pragma once

const int THRESHOLD = 128;

const int PREWITT_H_3x3[] = {-1, 0, 1, -1, 0, 1, -1, 0, 1};

const int PREWITT_V_3x3[] = {-1, -1, -1, 0, 0, 0, 1, 1, 1};

const int PREWITT_H_5x5[] = {9 , 9,  9,  9,  9,
                             9,  5,  5,  5,  9,
                            -7, -3,  0, -3, -7,
                            -7, -3, -3, -3, -7,
                            -7, -7, -7, -7, -7};

const int PREWITT_V_5x5[] = {9, 9, -7, -7, -7,
                             9, 5, -3, -3, -7,
                             9, 5,  0,  -3, -7,
                             9, 5, -3, -3, -7,
                             9, 9, -7, -7, -7};

struct pixel_grid{
    int start_w;
    int end_w;
    int start_h;
    int end_h;
};

int prewitt_convolve(int *, const int *, const int *, int, int, int, int);
int edge_detection_p_and_o(int *, int, int, int, int);

class Detector {
    private:
        int image_width;
        int image_height;

        int const *filter_h;
        int const *filter_v;
        int filter_size;

        int area;
        int cutoff;

    void edge_detection_helper(int *, int *, int, int, int);
    void prewitt_helper(int *, int *, int, int, int);

    public:
        Detector();
        ~Detector() {};

        void serial_prewitt(int *, int *, pixel_grid);
        void parallel_prewitt(int *, int *, pixel_grid);
        void serial_edge_detection(int *, int *, pixel_grid);
        void parallel_edge_detection(int *, int *, pixel_grid);

        void start_detector();
        void run_test_nr(int, BitmapRawConverter*, char*, int*, pixel_grid);

        void set_area(int);
        void set_cutoff(int);
        void set_image_width(int);
        void set_image_height(int);
        void set_detector(int);
        void set_filter_size(int);
};
