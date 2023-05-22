#include <iostream>
#include <stdlib.h>
#include <string>
#include "BitmapRawConverter.h"
#include <tbb/task_group.h>
#include "EasyBMP.h"

#define __ARG_NUM__				6
#define FILTER_SIZE				3
#define THRESHOLD				128

const int CUTOFF = 500;

using namespace std;
using namespace tbb;

int PICTURE_WIDTH = 0;
int PICTURE_HEIGHT = 0;

struct pixel_grid {
        int start_w;
        int end_w;
        int start_h;
        int end_h;
};

// Prewitt operators
int PREWITT_HORIZONTAL_3x3[FILTER_SIZE * FILTER_SIZE] = {-1, 0, 1, -1, 0, 1, -1, 0, 1};
int PREWITT_VERTICAL_3x3[FILTER_SIZE * FILTER_SIZE] = {-1, -1, -1, 0, 0, 0, 1, 1, 1};

int prewitt_convolve(int *input_matrix, const int *filter_h, const int *filter_v, int x, int y, int picture_size, int filter_size) {
    int picture_offset = (filter_size - 1) / 2;
    int vertical_sum = 0, horizontal_sum = 0;
    for(int i = 0; i < filter_size; ++i) {
        for(int j = 0; j < filter_size; ++j) {
            vertical_sum += filter_v[i * filter_size + j] * input_matrix[(x - picture_offset + i) * picture_size + (y - picture_offset + j)];
            horizontal_sum += filter_h[i * filter_size +j] * input_matrix[(x - picture_offset + i) * picture_size + (y - picture_offset + j)];
        }
    }
    return (abs(horizontal_sum) + abs(vertical_sum)) > THRESHOLD ? 255 : 0;
}

int edge_detection_p_and_o(int *input_matrix, int width, int x, int y, int area){
    int p = 0, o = 1;
    for(int i =0; i < area; i++) {
        for(int j = 0; j < area; j++) {
            if(input_matrix[(x - 1 + i) * width + (y - 1 + j)] >= THRESHOLD) p = 1;
            if(input_matrix[(x - 1 + i) * width + (y - 1 + j)] < THRESHOLD) o = 0;
        }
    }
    return abs(p-o) == 1 ? 255: 0;
}


void serial_prewitt(int *input_matrix, int *output_matrix, pixel_grid grid) {
    for(int i = grid.start_h; i < grid.end_h; ++i) {
        for(int j = grid.start_w; j < grid.end_w; ++j) {
            output_matrix[i * PICTURE_WIDTH + j] = prewitt_convolve(input_matrix, PREWITT_HORIZONTAL_3x3, PREWITT_VERTICAL_3x3, i, j, PICTURE_WIDTH, 3);
        }
    }
}

void parallel_prewitt(int *input_matrix, int *output_matrix, pixel_grid grid) {
    if (abs(grid.end_w - grid.start_w) <= CUTOFF || abs(grid.end_h - grid.start_h) <= CUTOFF){
        serial_prewitt(input_matrix, output_matrix, grid);
    }
    else {
        task_group tg;
        tg.run([&]() {
                pixel_grid g;
                g.start_w = grid.start_w;
                g.end_w = grid.start_w + (grid.end_w - grid.start_w) / 2;
                g.start_h = grid.start_h;
                g.end_h = grid.start_h + (grid.end_h - grid.start_h) / 2;
                parallel_prewitt(input_matrix, output_matrix, g);
        });
        tg.run([&]() {
                pixel_grid g;
                g.start_w = grid.start_w;
                g.end_w = grid.start_w + (grid.end_w - grid.start_w) / 2;
                g.start_h = grid.start_h + (grid.end_h - grid.start_h) / 2;
                g.end_h = grid.end_h;
                parallel_prewitt(input_matrix, output_matrix, g);
        });
        tg.run([&]() {
                pixel_grid g;
                g.start_w = grid.start_w + (grid.end_w - grid.start_w) / 2;
                g.end_w = grid.end_w;
                g.start_h = grid.start_h;
                g.end_h = grid.start_h + (grid.end_h - grid.start_h) / 2;
                parallel_prewitt(input_matrix, output_matrix, g);
        });
        tg.run([&]() {
                pixel_grid g;
                g.start_w = grid.start_w + (grid.end_w - grid.start_w) / 2;
                g.end_w = grid.end_w;
                g.start_h = grid.start_h + (grid.end_h - grid.start_h) / 2;
                g.end_h = grid.end_h;
                parallel_prewitt(input_matrix, output_matrix, g);
        });
        tg.wait();
    }
}

void edge_detection_serial(int *input_matrix, int *output_matrix, pixel_grid grid) {
    for(int i = grid.start_h; i < grid.end_h; ++i) {
        for(int j = grid.start_w; j < grid.end_w; ++j) {
            output_matrix[i * PICTURE_WIDTH + j] = edge_detection_p_and_o(input_matrix, PICTURE_WIDTH, i, j, 3) ;
        }
    }
}

void parallel_edge_detection(int *input_matrix, int *output_matrix, pixel_grid grid) {
    if (abs(grid.end_w - grid.start_w) <= CUTOFF || abs(grid.end_h - grid.start_h) <= CUTOFF){
        edge_detection_serial(input_matrix, output_matrix, grid);
    }
    else {
        task_group tg;
        tg.run([&]() {
                pixel_grid g;
                g.start_w = grid.start_w;
                g.end_w = grid.start_w + (grid.end_w - grid.start_w) / 2;
                g.start_h = grid.start_h;
                g.end_h = grid.start_h + (grid.end_h - grid.start_h) / 2;
                parallel_edge_detection(input_matrix, output_matrix, g);
        });
        tg.run([&]() {
                pixel_grid g;
                g.start_w = grid.start_w;
                g.end_w = grid.start_w + (grid.end_w - grid.start_w) / 2;
                g.start_h = grid.start_h + (grid.end_h - grid.start_h) / 2;
                g.end_h = grid.end_h;
                parallel_edge_detection(input_matrix, output_matrix, g);
        });
        tg.run([&]() {
                pixel_grid g;
                g.start_w = grid.start_w + (grid.end_w - grid.start_w) / 2;
                g.end_w = grid.end_w;
                g.start_h = grid.start_h;
                g.end_h = grid.start_h + (grid.end_h - grid.start_h) / 2;
                parallel_edge_detection(input_matrix, output_matrix, g);
        });
        tg.run([&]() {
                pixel_grid g;
                g.start_w = grid.start_w + (grid.end_w - grid.start_w) / 2;
                g.end_w = grid.end_w;
                g.start_h = grid.start_h + (grid.end_h - grid.start_h) / 2;
                g.end_h = grid.end_h;
                parallel_edge_detection(input_matrix, output_matrix, g);
        });
        tg.wait();
    }
}

void run_test_nr(int testNr, BitmapRawConverter* ioFile, char* outFileName, int* outBuffer, pixel_grid grid) {

	// TODO: start measure
	
	switch (testNr)
	{
		case 1:
            cout << "Running serial version of edge detection using Prewitt operator" << endl;
            serial_prewitt(ioFile->getBuffer(), outBuffer, grid);
			break;
		case 2:
			cout << "Running parallel version of edge detection using Prewitt operator" << endl;
			parallel_prewitt(ioFile->getBuffer(), outBuffer,grid);
			break;
		case 3:
			cout << "Running serial version of edge detection" << endl;
			edge_detection_serial(ioFile->getBuffer(), outBuffer, grid);
			break;
		case 4:
			cout << "Running parallel version of edge detection" << endl;
			parallel_edge_detection(ioFile->getBuffer(), outBuffer,grid);
			break;
		default:
			cout << "ERROR: invalid test case, must be 1, 2, 3 or 4!";
			break;
	}
	// TODO: end measure and display time

	ioFile->setBuffer(outBuffer);
	ioFile->pixelsToBitmap(outFileName);
}

int main(int argc, char * argv[])
{
	BitmapRawConverter inputFile("hk.bmp");
	BitmapRawConverter outputFileSerialPrewitt("out.bmp");
    BitmapRawConverter outputFileSerialEdge("out2.bmp");
	BitmapRawConverter outputFileParallelPrewitt("out3.bmp");
    BitmapRawConverter outputFileParallelEdge("out4.bmp");

	unsigned int width, height;

	int test;
	
	width = inputFile.getWidth();
	height = inputFile.getHeight();

    PICTURE_HEIGHT = height;
    PICTURE_WIDTH = width;

    int offset = (3-1)/2;
    pixel_grid grid;
    grid.start_h = offset;
    grid.start_w = offset;
    grid.end_h = height - offset;
    grid.end_w = width - offset;

    pixel_grid grid1, grid2, grid3, grid4;
    grid1 = grid;
    grid2 = grid;
    grid3 = grid;
    grid4 = grid;

	int* outBufferSerialPrewitt = new int[width * height];
	int* outBufferParallelPrewitt = new int[width * height];

	memset(outBufferSerialPrewitt, 0x0, width * height * sizeof(int));
	memset(outBufferParallelPrewitt, 0x0, width * height * sizeof(int));

	int* outBufferSerialEdge = new int[width * height];
	int* outBufferParallelEdge = new int[width * height];

	memset(outBufferSerialEdge, 0x0, width * height * sizeof(int));
	memset(outBufferParallelEdge, 0x0, width * height * sizeof(int));

	// serial version Prewitt
    //auto start = std::chrono::high_resolution_clock::now();
	run_test_nr(1, &outputFileSerialPrewitt, "out.bmp", outBufferSerialPrewitt,grid);
    //auto end = std::chrono::high_resolution_clock::now();
    //auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    //std::cout  << ",prewitt," << "serial,"<<  "," << elapsed << std::endl;

	// parallel version Prewitt
    run_test_nr(2, &outputFileParallelPrewitt, "out3.bmp", outBufferParallelPrewitt, grid);

	// serial version special
	run_test_nr(3, &outputFileSerialEdge, "out2.bmp", outBufferSerialEdge, grid);

	// parallel version special
	run_test_nr(4, &outputFileParallelEdge, "out4.bmp", outBufferParallelEdge, grid);

	// verification
	cout << "Verification: ";
	test = memcmp(outBufferSerialPrewitt, outBufferParallelPrewitt, width * height * sizeof(int));

	if(test != 0) {
		cout << "Prewitt FAIL!" << endl;
	} else { 
        cout << "Prewitt PASS." << endl;
	}

	test = memcmp(outBufferSerialEdge, outBufferParallelEdge, width * height * sizeof(int));

	if(test != 0) {
		cout << "Edge detection FAIL!" << endl;
	} else {
		cout << "Edge detection PASS." << endl;
	}

	// clean up
	delete outBufferSerialPrewitt;
	delete outBufferParallelPrewitt;
	delete outBufferSerialEdge;
	delete outBufferParallelEdge;
	return 0;
} 
