#include <iostream>
#include <vector>
#include <stdlib.h>
#include <string>
#include "bitmap/BitmapRawConverter.h"
#include "bitmap/EasyBMP.h"
#include <tbb/task_group.h>
#include "detector/detector.h"

using namespace std;
using namespace tbb;

int main(int argc, char * argv[])
{
    Detector d;
    d.start_detector();
    return 0;
} 
