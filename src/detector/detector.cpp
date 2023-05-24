#include "detector.h"
#include <iostream>

using namespace std;
using namespace tbb;

/**
 * @brief Detector constructor
 */
Detector::Detector() {}

/*
 * @brief Functions that starts edge detection process
 */
void Detector::start_detector(){
    vector<char*> images = {"../resources/hk.bmp",
                            "../resources/serial_prewitt.bmp",
                            "../resources/serial_edge.bmp",
                            "../resources/parallel_prewitt.bmp",
                            "../resources/parallel_edge.bmp"};

	BitmapRawConverter inputFile(images[0]);
	BitmapRawConverter outputFileSerialPrewitt(images[1]);
    BitmapRawConverter outputFileSerialEdge(images[2]);
	BitmapRawConverter outputFileParallelPrewitt(images[3]);
    BitmapRawConverter outputFileParallelEdge(images[4]);

    int width = inputFile.getWidth();
    int height = inputFile.getHeight();

	int* outBufferSerialPrewitt = new int[width * height];
	int* outBufferParallelPrewitt = new int[width * height];
	int* outBufferSerialEdge = new int[width * height];
	int* outBufferParallelEdge = new int[width * height];

    memset(outBufferSerialPrewitt, 0x0, width * height * sizeof(int));
    memset(outBufferParallelPrewitt, 0x0, width * height * sizeof(int));
	memset(outBufferSerialEdge, 0x0, width * height * sizeof(int));
	memset(outBufferParallelEdge, 0x0, width * height * sizeof(int));

    set_image_width(width);
    set_image_height(height);
    set_cutoff(800);
    set_filter_size(3);
    set_area(1);

    int offset = (this->filter_size-1)/2;
    pixel_grid grid;
    grid.start_h = offset;
    grid.start_w = offset;
    grid.end_h = height - offset;
    grid.end_w = width - offset;

	run_test_nr(1, &outputFileSerialPrewitt, images[1], outBufferSerialPrewitt,grid);
    run_test_nr(2, &outputFileParallelPrewitt, images[3], outBufferParallelPrewitt, grid);
	run_test_nr(3, &outputFileSerialEdge, images[2], outBufferSerialEdge, grid);
	run_test_nr(4, &outputFileParallelEdge, images[4], outBufferParallelEdge, grid);

	cout << "Verification: ";
	auto test = memcmp(outBufferSerialPrewitt, outBufferParallelPrewitt, width * height * sizeof(int));
	if(test != 0) { cout << "Prewitt FAIL!" << endl; } else { cout << "Prewitt PASS." << endl; }
	test = memcmp(outBufferSerialEdge, outBufferParallelEdge, width * height * sizeof(int));
	if(test != 0) { cout << "Edge detection FAIL!" << endl; } else { cout << "Edge detection PASS." << endl; }

	delete outBufferSerialPrewitt;
	delete outBufferParallelPrewitt;
	delete outBufferSerialEdge;
	delete outBufferParallelEdge;

}

/*
 * @brief Function that starts test for each method, writes changes to the file and prints time
 * @param test_number Number of the test, used in combination with case
 * @param io_file Buffer of the output image
 * @param out_file_name Name of the output file
 * @param out_buffer Matrix of the output file
 * @param grid image border structure
 */
void Detector::run_test_nr(int test_number, BitmapRawConverter* io_file, char* out_file_name, int* out_buffer, pixel_grid grid) {
    auto start = std::chrono::high_resolution_clock::now();
	switch (test_number)
	{
		case 1:
            cout << "Running serial version of edge detection using Prewitt operator" << endl;
            this->serial_prewitt(io_file->getBuffer(), out_buffer, grid);
			break;
		case 2:
			cout << "Running parallel version of edge detection using Prewitt operator" << endl;
			this->parallel_prewitt(io_file->getBuffer(), out_buffer,grid);
			break;
		case 3:
			cout << "Running serial version of edge detection" << endl;
			this->serial_edge_detection(io_file->getBuffer(), out_buffer, grid);
			break;
		case 4:
			cout << "Running parallel version of edge detection" << endl;
			this->parallel_edge_detection(io_file->getBuffer(), out_buffer,grid);
			break;
		default:
			cout << "ERROR: invalid test case, must be 1, 2, 3 or 4!";
			break;
	}
    auto end = std::chrono::high_resolution_clock::now();
    auto time_took = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
	io_file->setBuffer(out_buffer);
	io_file->pixelsToBitmap(out_file_name);
    cout <<"Time: " << time_took <<  " | Cutoff: " << this->cutoff << " | Distance:  " << this->filter_size <<  " | Area: "<< this->area << "."<<  endl; 
}

/*
 * @brief Function that applies Prewitt filter
 * @param input_matrix Input image buffer
 * @param filter_h Pointer to the horizontal prewitt operator
 * @param filter_v Pointer to the vertial prewitt operator
 * @param x Pixel width coordinate
 * @param y Pixel height coordinate
 * @param picture_size Width of the picture
 * @param filter_size Width of the prewitt filter matrix
 */
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

/*
 * @brief Function that calculate and manage p and o values for edge detection 
 * @param input_matrix Input image buffer
 * @param width Image width
 * @param x Pixel width coordinate
 * @param y Pixel height coordinate
 * @param filter_size Area around the pixel that is looked at
 */
int edge_detection_p_and_o(int *input_matrix, int width, int x, int y, int filter_size){
    int p = 0, o = 1;
    int picture_offset = (filter_size - 1) / 2;
    for(int i =0; i < filter_size; i++) {
        for(int j = 0; j < filter_size; j++) {
            if(input_matrix[(x - picture_offset + i) * width + (y - picture_offset + j)] >= THRESHOLD) p = 1;
            if(input_matrix[(x - picture_offset + i) * width + (y - picture_offset + j)] < THRESHOLD) o = 0;
        }
    }
    return abs(p-o) == 1 ? 255: 0;
}

/*
 * @brief Function that applies prewitt filter to the image
 * @param input_matrix Input image matrix
 * @param output_matrix Output image matrix
 * @param grid Image border structure
 */
void Detector::serial_prewitt(int *input_matrix, int *output_matrix, pixel_grid grid) {
    for(int i = grid.start_h; i < grid.end_h; ++i) {
        for(int j = grid.start_w; j < grid.end_w; ++j) {
            output_matrix[i * this->image_width + j] = prewitt_convolve(input_matrix, this->filter_h, this->filter_v, i, j, this->image_width, this->filter_size);
        }
    }
}

/*
 * @brief Function that implements parallel version of prewitt algorithm
 * @param input_matrix Input image matrix
 * @param output_matrix Output image matrix
 * @param grid Image border structure
 */
void Detector::parallel_prewitt(int *input_matrix, int *output_matrix, pixel_grid grid) {
    if (abs(grid.end_w - grid.start_w) <= this->cutoff || abs(grid.end_h - grid.start_h) <= this->cutoff){
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

/*
 * @brief Function that applies serial edge detection to the image
 * @param input_matrix Input image matrix
 * @param output_matrix Output image matrix
 * @param grid Image border structure
 */
void Detector::serial_edge_detection(int *input_matrix, int *output_matrix, pixel_grid grid) {
    for(int i = grid.start_h; i < grid.end_h; ++i) {
        for(int j = grid.start_w; j < grid.end_w; ++j) {
            output_matrix[i * this->image_width + j] = edge_detection_p_and_o(input_matrix, this->image_width, i, j, this->area) ;
        }
    }
}

/*
 * @brief Function that implements parallel version of edge detection algorithm
 * @param input_matrix Input image matrix
 * @param output_matrix Output image matrix
 * @param grid Image border structure
 */
void Detector::parallel_edge_detection(int *input_matrix, int *output_matrix, pixel_grid grid) {
    if (abs(grid.end_w - grid.start_w) <= this->cutoff || abs(grid.end_h - grid.start_h) <= this->cutoff){
        serial_edge_detection(input_matrix, output_matrix, grid);
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

/*
 * @brief Area setter
 * @param area Area around the pixel for edge detection
 */
void Detector::set_area(int area) {
    this->area = area * 2 + 1; 
}

/*
 * @brief Image height setter
 * @param height Height of the image
 */
void Detector::set_image_height(int height) {
    this->image_height = height; 
}

/*
 * @brief Image width setter
 * @param width Width of the image
 */
void Detector::set_image_width(int width) {
    this->image_width = width; 
}


/*
 * @brief Cutoff setter
 * @param cutoff Cutoff used by parallel algorithms
 */
void Detector::set_cutoff(int cutoff) {
    this->cutoff = cutoff; 
}

/*
 * @brief Filter Size setter
 * @param filter_size Value that is used when deciding Prewitt filter
 */
void Detector::set_filter_size(int filter_size) {
    this->filter_size = filter_size;
    if(filter_size == 3) {
        this->filter_h = PREWITT_H_3x3;
        this->filter_v = PREWITT_V_3x3;
    }
    if(filter_size == 5) {
        this->filter_h = PREWITT_H_5x5;
        this->filter_v = PREWITT_V_5x5;
    }
}
