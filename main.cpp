#include <iostream>
#include <stdlib.h>
#include <string>
#include "BitmapRawConverter.h"
#include <tbb/task_group.h>
#include "EasyBMP.h"

#define __ARG_NUM__				6
#define FILTER_SIZE				3
#define THRESHOLD				128

const int CUTOFF = 100;

using namespace std;
using namespace tbb;

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

void filter_serial_prewitt(int *input_matrix, int *output_matrix, int width, int height) {
    for(int i = 1; i < height; ++i) {
        for(int j = 1; j < width; ++j) {
            output_matrix[i * width + j] = prewitt_convolve(input_matrix, PREWITT_HORIZONTAL_3x3, PREWITT_VERTICAL_3x3, i, j, width, 3);
        }
    }
}

void edge_detection_serial(int *input_matrix, int *output_matrix, int width, int height) {
    for(int i = 1; i < height - 1; ++i) {
        for(int j = 1; j < width - 1; ++j) {
            output_matrix[i * width + j] = edge_detection_p_and_o(input_matrix, width, i, j, 3) ;
        }
    }
}

void filter_parallel_prewitt_helper(int *input_matrix, int *output_matrix, int width, int start_row, int end_row) {
    for (int i = start_row; i < end_row; ++i) {
        for (int j = 1; j < width - 1; ++j) {
            output_matrix[i * width + j] = prewitt_convolve(input_matrix, PREWITT_HORIZONTAL_3x3, PREWITT_VERTICAL_3x3, i, j, width, 3);
        }
    }
}

void filter_parallel_prewitt(int *input_matrix, int *output_matrix, int width, int height) {
    if (height <= CUTOFF || width <= CUTOFF) {
        filter_parallel_prewitt_helper(input_matrix, output_matrix, width, 1, height - 1);
    } else {
        task_group tasks;
        int mid = height / 2; 
        tasks.run([&]() { filter_parallel_prewitt(input_matrix, output_matrix, width, mid); });
        tasks.run([&]() { filter_parallel_prewitt(input_matrix + mid * width, output_matrix + mid * width, width, height - mid); });
        tasks.wait();
    }
}

void edge_detection_helper(int *input_matrix, int *output_matrix, int width, int start_row, int end_row) {
    for (int i = start_row; i < end_row; ++i) {
        for (int j = 1; j < width - 1; ++j) {
            output_matrix[i * width + j] = edge_detection_p_and_o(input_matrix, width, i, j, 3);
        }
    }
}

void filter_parallel_edge_detection(int *input_matrix, int *output_matrix, int width, int height) {
    if (height <= CUTOFF) {
        edge_detection_helper(input_matrix, output_matrix, width, 1, height - 1);
    } else {
        task_group tasks;
        int mid = height / 2; // Split the rows in half
        tasks.run([&]() { filter_parallel_edge_detection(input_matrix, output_matrix, width, mid); });
        tasks.run([&]() { filter_parallel_edge_detection(input_matrix + mid * width, output_matrix + mid * width, width, height - mid); });
        tasks.wait();
    }
}

void run_test_nr(int testNr, BitmapRawConverter* ioFile, char* outFileName, int* outBuffer, unsigned int width, unsigned int height) {

	// TODO: start measure
	
	switch (testNr)
	{
		case 1:
            cout << "Running serial version of edge detection using Prewitt operator" << endl;
            filter_serial_prewitt(ioFile->getBuffer(), outBuffer, width, height);
			break;
		case 2:
			cout << "Running parallel version of edge detection using Prewitt operator" << endl;
			filter_parallel_prewitt(ioFile->getBuffer(), outBuffer, width, height);
			break;
		case 3:
			cout << "Running serial version of edge detection" << endl;
			edge_detection_serial(ioFile->getBuffer(), outBuffer, width, height);
			break;
		case 4:
			cout << "Running parallel version of edge detection" << endl;
			filter_parallel_edge_detection(ioFile->getBuffer(), outBuffer, width, height);
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
	BitmapRawConverter inputFile("color.bmp");
	BitmapRawConverter outputFileSerialPrewitt("out.bmp");
	BitmapRawConverter outputFileParallelPrewitt("out3.bmp");
    BitmapRawConverter outputFileSerialEdge("out2.bmp");
    BitmapRawConverter outputFileParallelEdge("out4.bmp");

	unsigned int width, height;

	int test;
	
	width = inputFile.getWidth();
	height = inputFile.getHeight();

	int* outBufferSerialPrewitt = new int[width * height];
	int* outBufferParallelPrewitt = new int[width * height];

	memset(outBufferSerialPrewitt, 0x0, width * height * sizeof(int));
	memset(outBufferParallelPrewitt, 0x0, width * height * sizeof(int));

	int* outBufferSerialEdge = new int[width * height];
	int* outBufferParallelEdge = new int[width * height];

	memset(outBufferSerialEdge, 0x0, width * height * sizeof(int));
	memset(outBufferParallelEdge, 0x0, width * height * sizeof(int));

	// serial version Prewitt
	run_test_nr(1, &outputFileSerialPrewitt, "out.bmp", outBufferSerialPrewitt, width, height);

	// parallel version Prewitt
    run_test_nr(2, &outputFileParallelPrewitt, "out3.bmp", outBufferParallelPrewitt, width, height);

	// serial version special
	run_test_nr(3, &outputFileSerialEdge, "out2.bmp", outBufferSerialEdge, width, height);

	// parallel version special
	run_test_nr(4, &outputFileParallelEdge, "out4.bmp", outBufferParallelEdge, width, height);

	// verification
	cout << "Verification: ";
	test = memcmp(outBufferSerialPrewitt, outBufferParallelPrewitt, width * height * sizeof(int));

	if(test != 0)
	{
		cout << "Prewitt FAIL!" << endl;
	}
	else
	{
		cout << "Prewitt PASS." << endl;
	}

	test = memcmp(outBufferSerialEdge, outBufferParallelEdge, width * height * sizeof(int));

	if(test != 0)
	{
		cout << "Edge detection FAIL!" << endl;
	}
	else
	{
		cout << "Edge detection PASS." << endl;
	}

	// clean up
	delete outBufferSerialPrewitt;
	delete outBufferParallelPrewitt;

	delete outBufferSerialEdge;
	delete outBufferParallelEdge;

	return 0;
} 
